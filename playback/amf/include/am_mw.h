
/**
 * am_mw.h
 *
 * History:
 *    2009/12/7 - [Oliver Li] created file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_MW_H__
#define __AM_MW_H__

#include "am_dsp_if.h"
class CBuffer;
class CVideoBuffer;
class CAudioBuffer;
class CMediaFormat;
//-----------------------------------------------------------------------
//
// CBuffer
//
//-----------------------------------------------------------------------

class CBuffer
{
public:
	enum Type {
		DATA,
		INFO,
		EOS,
		FLOW_CONTROL,//general flow control use
		UDEC_STATUS,//dsp use only
		TEST_ES,
	};

	enum DataFlags {
		SYNC_POINT = (1 << 0),
	};

public:
	am_atomic_t	mRefCount;

	AM_U8		mType;		// Type
	AM_U8		mFlags;		// Flags

	AM_U8		mFrameType;	//
	AM_U32		mReserved;	//

	AM_INTPTR	mTag;

	AM_UINT		mBlockSize;
	AM_UINT		mDataSize;

	AM_UINT		mSeqNum;
	AM_U8		*mpData;
	AM_U8		*mpDataBase;

    AM_U64  mPTS;
    AM_U64  mExpireTime;//life time, for recording
    AM_UINT mOriSeqNum;//from dsp/iav
    AM_U16  mBufferId;  //
    AM_U16  mEncId; //

	IBufferPool	*mpPool;
	CBuffer		*mpNext;

        //////
        AM_INTPTR mpExtra;
        void SetExtraPtr(AM_INTPTR ptr) { mpExtra = ptr; }
        AM_INTPTR GetExtraPtr() const { return mpExtra; }

	void SetDataPtr(AM_U8 *ptr) { mpData = ptr; }
	AM_U8 *GetDataPtr() { return mpData; }

	AM_UINT GetDataSize() { return mDataSize; }
	void SetDataSize(AM_UINT size) { mDataSize = size; }
	void IncDataSize(AM_UINT size) { mDataSize += size; }

	void AddRef() { mpPool->AddRef(this); }
	void Release() {mpPool->Release(this);}

	AM_INT GetType() { return mType; }
	void SetType(AM_INT type) { mType = type; }

	AM_INT GetFlags() { return mFlags; }
	void SetFlags(AM_INT flags) { mFlags = flags; }

	AM_U64 GetPTS() { return mPTS; }
	void SetPTS(AM_U64 pts) { mPTS = pts; }

	bool IsEOS() { return GetType() == EOS; }
};

class CVideoBuffer: public CBuffer
{
public:
	AM_U8		*pLumaAddr;
	AM_U8		*pChromaAddr;
	AM_INTPTR	tag;
    AM_INT	    real_buffer_id;
    AM_INT      buffer_id;
	AM_UINT		picWidth;
	AM_UINT		picHeight;
	AM_UINT		fbWidth;
	AM_UINT		fbHeight;
	AM_UINT		picXoff;
	AM_UINT		picYoff;
	AM_UINT		flags;
	//AM_INT		fr_num;	//num of frame rate
	//AM_INT		fr_den;	//den of frame rate, num / den is frame rate.
};

class CAudioBuffer: public CBuffer
{
public:
    AM_UINT	sampleRate;
    AM_UINT	numChannels;
    AM_UINT	frameSize;
    AM_UINT	audioType;//codec_id avcodec
    AM_INT sampleFormat;
    AM_INTPTR	tag;
};

enum SUBTITLETYPE
{
    SUBTYPE_NON,
    SUBTYPE_XSUB,
    SUBTYPE_DVDSUP,
    SUBTYPE_TEXT,
    SUBTYPE_ASS,
    SUBTYPE_OTHER
};

class CSubtitleBuffer: public CBuffer
{
public:
    SUBTITLETYPE	subType;
    AM_U64	start_time;
    AM_U64	end_time;
    //for ASS/SSA
    AM_U8 style[128];
    AM_U8 fontname[128];
    AM_UINT fontsize;
    AM_U64 PrimaryColour, BackColour;
    AM_UINT Bold, Italic;
    CSubtitleBuffer();
    //virtual ~CSubtitleBuffer() {}
};

//-----------------------------------------------------------------------
//
// CMediaFormat
//
//-----------------------------------------------------------------------
enum {
    PreferWorkMode_None = 0,
    PreferWorkMode_UDEC,
    PreferWorkMode_CameraRecording,
    PreferWorkMode_Duplex,
};

struct CMediaFormat
{
    const AM_GUID	*pMediaType;
    const AM_GUID	*pSubType;
    const AM_GUID	*pFormatType;
    AM_UINT         formatSize;
    AM_INTPTR      format;
    AM_INT           mDspIndex;
    AM_UINT        reserved1;

    AM_U16         preferWorkMode;//for choose DSP work mode
    AM_U16         preferFilter;//for choose filter

    CMediaFormat()
    {
        pMediaType = &GUID_NULL;
        pSubType = &GUID_NULL;
        pFormatType = &GUID_NULL;
        formatSize = 0;
        format = 0;
        reserved1 = 0;
        preferWorkMode = PreferWorkMode_None;
        preferFilter = 0;
    }
};

//======================================================
//
//
//======================================================
#define D_DSP_VIDEO_BUFFER_WIDTH_ALIGNMENT 32
#define D_DSP_VIDEO_BUFFER_HEIGHT_ALIGNMENT 32

typedef struct
{
    AM_UINT codecId;
    AM_UINT dectype;
} DECSETTING;

typedef struct
{
    AM_UINT audio_disable, video_disable, subtitle_disable, avsync_enable, vout_config, ar_enable, quickAVSync_enable, auto_vout_enable;
    AM_UINT not_config_audio_path, not_config_video_path;
    AM_UINT input_buffer_number;//hybird mpeg4
} SPBConfig;

//udec/dsp related parameters
#ifdef CONFIG_AM_ENABLE_MDEC
#define DMAX_UDEC_INSTANCE_NUM 16
#else
#define DMAX_UDEC_INSTANCE_NUM 2
#endif

typedef struct {
    AM_U8   postp_mode;   // 0: no pp; 1: decpp; 2: voutpp
    AM_U8   enable_deint;   // 1: enable deinterlacer
    AM_U8   pp_chroma_fmt_max;  // 0: mon; 1: 420; 2: 422; 3: 444

    AM_U8   pp_max_frm_width;
    AM_U8   pp_max_frm_height;
    AM_U8   pp_max_frm_num;

    AM_U8   vout_mask;  // bit 0: use VOUT 0; bit 1: use VOUT 1
    AM_U8   num_udecs;
} DSPUdecModeConfig;

typedef struct {
    AM_INT enable;//video layer
    AM_INT osd_disable;//osd layer
    AM_INT sink_type;
    AM_INT sink_mode;
    AM_INT sink_id;
    AM_INT width, height;
    AM_INT pos_x, pos_y;
    AM_INT size_x, size_y;
    AM_INT video_x, video_y;
    AM_U32 zoom_factor_x, zoom_factor_y;
    AM_INT flip, rotate;
    AM_INT failed;//checking/opening failed
} DSPVoutConfig;

typedef struct DSPVoutConfigs{
    DSPVoutConfig voutConfig[eVoutCnt];

    //vout common parameters
    AM_UINT vout_mask;
    AM_UINT vout_start_index;
    AM_UINT num_vouts;
    AM_INT input_center_x, input_center_y;
    AM_U16   pp_max_frm_width;
    AM_U16   pp_max_frm_height;

    //current source rect, related to zoom_factor
    AM_INT src_pos_x, src_pos_y;
    AM_INT src_size_x, src_size_y;

    //picture related
    AM_INT picture_offset_x;
    AM_INT picture_offset_y;
    AM_INT picture_width;
    AM_INT picture_height;

    //need restore
    AM_INT need_restore_osd[eVoutCnt];

    //Mudec: second vout size
    AM_INT second_vout_width;
    AM_INT second_vout_height;
} DSPVoutConfigs;

typedef struct {
    //de-interlace parameters
    AM_U8 init_tff; //top_field_first   1
    AM_U8 deint_lu_en; // 1
    AM_U8 deint_ch_en; // 1
    AM_U8 osd_en; // 0

    AM_U8 deint_mode; //for 1080i60 input, you need to set '1' here, for 480i/576i input, you can set it to '0' or '1'
    AM_U8 deint_reserved[3];

    AM_U8 deint_spatial_shift; //0
    AM_U8 deint_lowpass_shift; //7

    AM_U8 deint_lowpass_center_weight; // 112
    AM_U8 deint_lowpass_hor_weight; // 2
    AM_U8 deint_lowpass_ver_weight; // 4

    AM_U8 deint_gradient_bias; // 15
    AM_U8 deint_predict_bias; // 15
    AM_U8 deint_candidate_bias; // 10

    AM_S16 deint_spatial_score_bias; // 5
    AM_S16 deint_temporal_score_bias; // 5
} DSPDeinterlaceConfig;

//hard code here, for libaacenc.lib
#define AUDIO_CHUNK_SIZE 1024
#define SINGLE_CHANNEL_MAX_AUDIO_BUFFER_SIZE (AUDIO_CHUNK_SIZE*sizeof(AM_S16))
#define MAX_AUDIO_BUFFER_SIZE (SINGLE_CHANNEL_MAX_AUDIO_BUFFER_SIZE*2)


// config DSPUdecInstanceConfig::frm_chroma_fmt_max
#define UDEC_CFG_FRM_CHROMA_FMT_420    0
#define UDEC_CFG_FRM_CHROMA_FMT_422    1
#define TICK_PER_SECOND    90000

typedef struct {
    AM_U8   tiled_mode; // tiled mode, 0, 4, 5
    AM_U8   frm_chroma_fmt_max; // 0: 420; 1: 422
    AM_U8   dec_types;
    AM_U8   max_frm_num;
    AM_U16  max_frm_width;
    AM_U16  max_frm_height;
    AM_U32  max_fifo_size;

    AM_U8   udec_id;
    AM_U8   udec_type;
    AM_U8   enable_pp;
    AM_U8   enable_deint;

    // the vout used by this udec
    AM_U8   interlaced_out;
    AM_U8   packed_out; // 0: planar yuv; 1: packed yuyv
    AM_U8   enable_err_handle;
    AM_U8   reserved;
    AM_U32  bits_fifo_size;
    AM_U32  ref_cache_size;

    AM_U16  concealment_mode;   // 0 - use the last good picture as the concealment reference frame
                                // 1 - use the last displayed as the concealment reference frame
                                // 2 - use the frame given by ARM as the concealment reference frame
    AM_U32  concealment_ref_frm_buf_id; // used only when concealment_mode is 2

    //out
    AM_U8* pbits_fifo_start;
} DSPUdecInstanceConfig;

typedef struct {
    //error handling config
    AM_U16 enable_udec_error_handling;
    AM_U16 error_concealment_mode;
    AM_U16 error_concealment_frame_id;
    AM_U16 error_handling_app_behavior;//debug use, 0:default(stop(1)-->restart decoding), 1:always STOP(0), and exit, 2: halt, not touch env, for debug
} DSPUdecErrorHandlingConfig;

typedef struct {
    AM_U32 pquant_mode;
    AM_U16 pquant_table[32];
} DSPUdecDeblockingConfig;

enum {
     eAddVideoDataType_none = 0,
     eAddVideoDataType_iOneUDEC = 1,
     eAddVideoDataType_a5sH264GOPHeader = 2,
};

typedef struct DSPConfig{
    DSPUdecModeConfig modeConfig;
    DSPDeinterlaceConfig deinterlaceConfig;
    DSPUdecInstanceConfig udecInstanceConfig[DMAX_UDEC_INSTANCE_NUM];
    DSPUdecErrorHandlingConfig errorHandlingConfig[DMAX_UDEC_INSTANCE_NUM];
    DSPVoutConfigs voutConfigs;
    DSPUdecDeblockingConfig deblockingConfig;

    AM_INT hdWin;
    AM_INT voutMask;//user request vout mask
    AM_INT second_vout_scale_mode;//only for Mudec dual vout case
    AM_INT curInstanceIsHd;
    //generic settings
    AM_UINT addVideoDataType;
    AM_UINT pre_buffer_len;
    AM_UINT enable_dsp_pre_ctrl;

    AM_UINT enableDeinterlace;
    AM_UINT deblockingFlag;
    AM_UINT getoutPic[DMAX_UDEC_INSTANCE_NUM];//get_pic out from dsp
    AM_UINT decoderType[DMAX_UDEC_INSTANCE_NUM];

    //
    AM_U8 preset_tilemode;
    AM_U8 preset_enable_prefetch;
    AM_U8 preset_prefetch_count;
    AM_U8 reserved2;
    AM_U32 preset_bits_fifo_size;
    AM_U32 preset_ref_cache_size;
} DSPConfig;

typedef struct TranscConfig{
    AM_UINT video_w[DMAX_UDEC_INSTANCE_NUM];
    AM_UINT video_h[DMAX_UDEC_INSTANCE_NUM];
    AM_UINT dec_w[DMAX_UDEC_INSTANCE_NUM];
    AM_UINT dec_h[DMAX_UDEC_INSTANCE_NUM];
    AM_UINT enc_w;
    AM_UINT enc_h;
    AM_UINT bitrate;
    AM_UINT chroma_fmt;//0: 420; 1: 422
    bool enableTranscode;
    bool keepAspectRatio;
} TranscConfig;

enum {
     ePriDataParse_disable = 0,
     ePriDataParse_filter_parse,
     ePriDataParse_engine_demuxer_post,
};

//dsp related
#define DUDEC_MAX_SEQ_CONFIGDATA_LEN 40
#define DUDEC_MAX_PIC_CONFIGDATA_LEN 8

enum H264_DATA_FORMAT {
    H264_FMT_INVALID,
    H264_FMT_AVCC,      // nal delimit: 4 byte, which represents data length
    H264_FMT_ANNEXB     // nal delimiter: 00 00 00 01
};

#if 0
//should only contain some persistant configuration
//or should only one thread write it
//or should use mutex to protect them
typedef struct
{
//settings:
    //pb-engine general settings
    SPBConfig pbConfig;

    //set video decoder type
    DECSETTING decSet[CODEC_NUM];
    AM_UINT mdecCap[CODEC_NUM];
    AM_UINT decCurrType;
    AM_UINT is_avi_flag;
    AM_UINT is_mp4s_flag;//2011.09.23 has resolution info case by roy
    AM_INT vid_container_width;
    AM_INT vid_container_height;

    //ppmode
    AM_UINT ppmode;
    AM_UINT get_outpic;//get_pic out from dsp

    //log config
//    SLogConfig logConfig[DPB_MAX_AO_NUM];
//#ifdef AM_DEBUG
//    AM_U8 log_prt_index;
//#endif
    //av sync parameters
    AM_UINT clockTimebaseNum;
    AM_UINT clockTimebaseDen;

    //error handling variable
    AM_UINT udec_state;
    AM_UINT vout_state;
    AM_UINT error_code;

//shared resource
    //todo

    //shared iav fd
    AM_INT mbIavInited;
    AM_INT mIavFd;

    //flush clear
    AM_UINT mbAlreadySynced;

    //shared data, please make sure only one thread write them
    am_pts_t mPlaybackStartTime;//estimated start pts just after seek, always valid, only write by engine

    bool mbDurationEstimated;//added by liujian, duration estimated by ffmpeg_demux
    bool videoEnabled;
    bool audioEnabled;

    //canbe write only in demux thread
    bool mbVideoFirstPTSVaild;
    bool mbAudioFirstPTSVaild;
    am_pts_t videoFirstPTS; //first video packet's PTS from demuxer
    am_pts_t audioFirstPTS; //first audio packet's PTS from demuxer

    //frame distance, used for generate pts
    AM_UINT mVideoTicks;
    AM_UINT mAudioTicks;

    AM_U8 mbSeekFailed;
    AM_U8 mbPaused;
    AM_U8 mbLoopPlay;
    AM_U8 mbRandomNextFile;

//    AM_UINT mUdecState;

    AM_UINT mbStartWithStepMode;
    AM_UINT mStepCnt;

    //debug flag
    AM_UINT mDecoderSelect;//0:nv12 pipe line first, 1:nv12 first, 2: original 420p decoder
    AM_UINT mGeneralDecoder;
    AM_UINT mDebugFlag;
    AM_UINT mForceLowdelay;//skip B picture
    //must be consist
    AM_UINT mPridataParsingMethod;//0 disable parsing, 1 use parser filter, 2 engine post data(not use filter)

    //debug use
    AM_UINT force_decode;// 1: force decode when seeking to Non-key frame
    AM_UINT validation_only;//designed for video editing
#ifdef AM_DEBUG
    pthread_mutex_t mMutex;
#endif
    //debug use, to be removed in future
    AM_INT mEngineVersion;//0:pb-engine, 1:avctive-engine

    //dsp related
    DSPConfig dspConfig;
    IUDECHandler* udecHandler;
    IVoutHandler* voutHandler;

    AM_INT mDSPmode;//0: default, 1udec, playback; 1: ppmode=1, run multi-instance, used in VE case.3: transcode
    AM_INT udecNums;

    AM_INT mReConfigMode; //0: when reconfig decoder, just generate new extra data and change input center 1: need to initUdec

    //simple strategy for avoid data overflow sent to app
    volatile int mnPrivateDataCountNeedSent;

    //debug only, read only config
    AM_INT play_video_es;
    AM_INT also_demux_file;
    AM_UINT specified_framerate_num;
    AM_UINT specified_framerate_den;

    TranscConfig sTranscConfig;//transcode/editing use

    // deinterlace config
    bool bEnableDeinterlace;
}SharedResource;
#endif

typedef struct {
    AM_U32 factor_x, factor_y;
    SRect rect[2];//0 src, 1 dest
} SDisplayRectMap;


//platfrom related,tmp here
#define DMAX_VIDEO_ENC_STREAM_NUMBER 2

typedef struct
{
    AM_UINT container_format;
    AM_UINT video_format, audio_format;
    AM_UINT pic_width, pic_height;
    AM_UINT channel_number, sample_format, sample_rate;
    AM_UINT video_bitrate, audio_bitrate;
    AM_UINT video_framerate_num, video_framerate_den;
    AM_UINT M, N, IDRInterval;
    AM_UINT video_lowdelay;
    IParameters::EntropyType entropy_type;
} STargetRecorderConfig;

typedef struct {
    AM_U16 main_win_width;
    AM_U16 main_win_height;
    AM_U16 dsp_mode;
    AM_U16 stream_number;

    AM_U16 enc_width, enc_height;
    AM_U16 enc_offset_x, enc_offset_y;
    AM_U16 second_enc_width, second_enc_height;

    AM_UINT M, N, IDRInterval;
    AM_UINT video_bitrate;
    IParameters::EntropyType entropy_type;

    AM_U8 num_of_enc_chans;
    AM_U8 num_of_dec_chans;
    //vout related
    AM_U8 playback_vout_index;
    AM_U8 playback_in_pip;

    AM_U8 preview_enabled;
    AM_U8 preview_vout_index;
    AM_U8 preview_alpha;
    AM_U8 preview_in_pip;

    AM_U16 preview_left;
    AM_U16 preview_top;
    AM_U16 preview_width;
    AM_U16 preview_height;

    AM_U16 pb_display_left;
    AM_U16 pb_display_top;
    AM_U16 pb_display_width;
    AM_U16 pb_display_height;

    AM_U8 pb_display_enabled;

    //previewc related
    AM_U8 previewc_rawdata_enabled;
    AM_U8 thumbnail_enabled;
    AM_U8 dsp_piv_enabled;
    AM_U16 previewc_scaled_width;
    AM_U16 previewc_scaled_height;

    AM_U16 previewc_crop_offset_x;
    AM_U16 previewc_crop_offset_y;
    AM_U16 previewc_crop_width;
    AM_U16 previewc_crop_height;

    AM_U16 thumbnail_width;
    AM_U16 thumbnail_height;

    AM_U16 dsp_jpeg_width;
    AM_U16 dsp_jpeg_height;

    AM_U8 vcap_ppl_type;
    AM_U8 reserved0;
    AM_U8 reserved1;
    AM_U8 reserved2;
} SEncodingModeConfig;

typedef struct {
    AM_U16 rtsp_listen_port;
    AM_U16 rtp_rtcp_port_start;
} SRTSPServerConfig;

//notification msg, skychen 2012_11_21
typedef struct {
    AM_UINT flag;//0:close msg notification
    AM_U16 msg_server_listen_port;
    AM_U16 msg_client_local_port;
    char msg_server_ip_addr[64];
} SMSGServerConfig;

//debug use
typedef struct _SConsistentConfig
{
    //playback related

    //pb-engine general settings
    SPBConfig pbConfig;

    //set video decoder type
    DECSETTING decSet[CODEC_NUM];
    AM_UINT mdecCap[CODEC_NUM];
    AM_UINT decCurrType;
    AM_UINT is_avi_flag;
    AM_UINT is_mp4s_flag;//2011.09.23 has resolution info case by roy
    AM_INT vid_container_width;
    AM_INT vid_container_height;
    AM_INT vid_width_DAR;//for bug#2105 #2322, save the width adjusted to keep DAR when SAR!=1:1
    AM_INT vid_rotation_info;//roy mdf 2012.03.21

    //ppmode
    AM_UINT ppmode;
    AM_UINT get_outpic;//get_pic out from dsp

    //log config
//    SLogConfig logConfig[DPB_MAX_AO_NUM];
//#ifdef AM_DEBUG
//    AM_U8 log_prt_index;
//#endif
    //av sync parameters
    AM_UINT clockTimebaseNum;
    AM_UINT clockTimebaseDen;

    //error handling variable
    AM_UINT udec_state;
    AM_UINT vout_state;
    AM_UINT error_code;

//shared resource
    //todo

    //shared iav fd
    AM_INT mbIavInited;
    AM_INT mIavFd;

    //flush clear
    AM_UINT mbAlreadySynced;

    //shared data, please make sure only one thread write them
    am_pts_t mPlaybackStartTime;//estimated start pts just after seek, always valid, only write by engine

    bool mbDurationEstimated;//added by liujian, duration estimated by ffmpeg_demux
    bool videoEnabled;
    bool audioEnabled;

    //canbe write only in demux thread
    bool mbVideoFirstPTSVaild;
    bool mbAudioFirstPTSVaild;
    am_pts_t videoFirstPTS; //first video packet's PTS from demuxer when start or seek
    am_pts_t audioFirstPTS; //first audio packet's PTS from demuxer when start or seek

    //canbe write only in demux thread when LoadFile
    bool mbVideoStreamStartTimeValid;
    bool mbAudioStreamStartTimeValid;
    am_pts_t videoStreamStartPTS; //first video frame Start PTS from demuxer
    am_pts_t audioStreamStartPTS; //first audio sample Start PTS from demuxer

    //frame distance, used for generate pts
    AM_UINT mVideoTicks;
    AM_UINT mAudioTicks;

    AM_U8 mbSeekFailed;
    AM_U8 mbPaused;
    AM_U8 mbLoopPlay;
    AM_U8 mbRandomNextFile;

//    AM_UINT mUdecState;

    AM_UINT mbStartWithStepMode;
    AM_UINT mStepCnt;

    //debug flag
    AM_UINT mDecoderSelect;//0:nv12 pipe line first, 1:nv12 first, 2: original 420p decoder
    AM_UINT mGeneralDecoder;
    AM_UINT mDebugFlag;
    AM_UINT mForceLowdelay;//skip B picture
    //must be consist
    AM_UINT mPridataParsingMethod;//0 disable parsing, 1 use parser filter, 2 engine post data(not use filter)

    //debug use
    AM_UINT force_decode;// 1: force decode when seeking to Non-key frame
    AM_UINT validation_only;//designed for video editing
#ifdef AM_DEBUG
    pthread_mutex_t mMutex;
#endif
    //debug use, to be removed in future
    AM_INT mEngineVersion;//0:pb-engine, 1:avctive-engine

    //dsp related
    DSPConfig dspConfig;
    IUDECHandler* udecHandler;
    IVoutHandler* voutHandler;

    //dsp debug related
    AM_U32 codec_mask;
    AM_U8 enable_feature_constrains;
    AM_U8 always_disable_l2_cache;
    AM_U8 max_frm_number;
    AM_U8 noncachable_buffer;
    AM_U8 h264_no_fmo;
    AM_U8 vout_no_LCD;
    AM_U8 vout_no_HDMI;
    AM_U8 no_deinterlacer;

    //debug related
    AM_U8 discard_half_audio_packet;
    AM_U8 auto_start_rtsp_steaming;

    AM_INT mDSPmode;//0: default, 1udec, playback; 1: ppmode=1, run multi-instance, used in VE case.3: transcode
    AM_INT udecNums;

    AM_INT mReConfigMode; //0: when reconfig decoder, just generate new extra data and change input center 1: need to initUdec

    //simple strategy for avoid data overflow sent to app
    volatile int mnPrivateDataCountNeedSent;

    //debug only, read only config
    AM_INT play_video_es;
    AM_INT also_demux_file;
    AM_UINT specified_framerate_num;
    AM_UINT specified_framerate_den;

    TranscConfig sTranscConfig;//transcode/editing use

    // deinterlace config
    bool bEnableDeinterlace;
    //recorder config
    AM_UINT video_enable, audio_enable, subtitle_enable, private_data_enable, private_gps_sub_info_enable, private_am_trickplay_enable;
    AM_UINT streaming_enable, cutfile_with_precise_pts, streaming_video_enable, streaming_audio_enable;

    AM_UINT tot_muxer_number;
    STargetRecorderConfig target_recorder_config[DMAX_VIDEO_ENC_STREAM_NUMBER];

    //dsp mode config, encoding/duplex
    SEncodingModeConfig encoding_mode_config;

    //debug use
    AM_UINT not_start_encoding;
    AM_UINT muxer_dump_audio, muxer_dump_video, muxer_dump_pridata, muxer_skip_video, muxer_skip_audio, muxer_skip_pridata, not_check_video_res;//debug use
    AM_UINT use_itron_filter, video_from_itron, audio_from_itron;
    AM_UINT enable_fade_in;

    AM_U8 use_force_play_item;
    AM_U8 auto_purge_buffers_before_play_rtsp;
    AM_U8 enable_horz_dewarp;
    AM_U8 select_camera_index;

    SRTSPServerConfig rtsp_server_config;

    //some string parameters
    char force_play_url[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN + 4];

    //notification msg
    SMSGServerConfig msg_server_config;

    //color test
    AM_UINT enable_color_test;
    AM_UINT color_test_number;
    AM_U32 color_ycbcralpha[32];

    //added by jianliu for NVR rtsp streaming,
    //     iOne record_engine  should support the case that rtsp-server works without saving files to sdcard.
    int disable_save_files;
} SConsistentConfig;

//SharedResource is obsolete, please remove it
typedef SConsistentConfig SharedResource;

//YUV data
typedef struct _SYUVData
{
    AM_INT buffer_id;
    AM_UINT timestamp;
    AM_UINT type;
    AM_INT image_width;
    AM_INT image_height;
    AM_INT image_step;
    void* yData;
    void* uvData;
}SYUVData;
#endif

