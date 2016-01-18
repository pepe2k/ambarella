
/**
 * am_if.h
 *
 * History:
 *    2007/11/5 - [Oliver Li] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_IF_H__
#define __AM_IF_H__

extern const AM_IID IID_IInterface;
extern const AM_IID IID_IActiveObject;
extern const AM_IID IID_IMSGSYS;
extern const AM_IID IID_IMsgPort;
extern const AM_IID IID_IMsgSink;

extern const AM_IID IID_IBufferPool;
extern const AM_IID IID_IPin;
extern const AM_IID IID_IFilter;
extern const AM_IID IID_IParameters;
extern const AM_IID IID_IDemuxer;
extern const AM_IID IID_IDecoder;
extern const AM_IID IID_IEditor;
extern const AM_IID IID_IRenderer;
extern const AM_IID IID_IDataRetriever;

extern const AM_IID IID_IEngine;
extern const AM_IID IID_IVideoOutput;
extern const AM_IID IID_IMediaControlOnTheFly;
extern const AM_IID IID_IMediaControl;
extern const AM_IID IID_IStreamingControl;
extern const AM_IID IID_ISimplePlayback;

extern const AM_IID IID_IClockSource;
extern const AM_IID IID_IClockObserver;
extern const AM_IID IID_IClockManager;
extern const AM_IID IID_IClockManagerExt;

extern const AM_IID IID_IUdecHandler;
extern const AM_IID IID_IVoutHandler;

extern const AM_IID IID_IFileReader;
extern const AM_IID IID_IAudioHAL;
extern const AM_IID IID_IDecoderControl;

extern const AM_IID IID_IAudioSink;

extern const AM_IID IID_IWatcher;

class IInterface;
class IActiveObject;
class IMsgPort;
class IMsgSink;

class IBufferPool;
class IPin;
class IFilter;
class IDemuxer;
class IDecoder;
class IEditor;
class IRender;

class IEngine;
class IMediaControl;

class IClockSource;
class IClockObserver;
class IClockManager;

class IFileReader;

class CBuffer;
class CMediaFormat;
class CMsgSys;

struct AM_MSG
{
    AM_UINT     code;
    AM_UINT     sessionID;
    AM_INTPTR   p0;
    AM_INTPTR   p1;
    AM_INTPTR   p2;
    AM_INTPTR   p3;
    AM_U8       p4, p5, p6, p7;
    AM_UINT    reserved0;
};


#define DECLARE_INTERFACE(ifName, iid) \
	static ifName* GetInterfaceFrom(IInterface *pInterface) \
	{ \
		return (ifName*)pInterface->GetInterface(iid); \
	}


typedef AM_U64 AM_PTS;
//-----------------------------------------------------------------------
//
// IInterface
//
//-----------------------------------------------------------------------
class IInterface
{
public:
	virtual void *GetInterface(AM_REFIID refiid) = 0;
	virtual void Delete() = 0;
	virtual ~IInterface() {}
};
//-----------------------------------------------------------------------
//
// IActiveObject
//
//-----------------------------------------------------------------------
class IActiveObject: public IInterface
{
public:
    enum {
        CMD_TERMINATE,
        CMD_RUN,
        CMD_STOP,
        CMD_START,
        CMD_PAUSE,
        CMD_RESUME,
        CMD_FLUSH,
        CMD_BEGIN_PLAYBACK,//after seek
        CMD_FORCE_LOW_DELAY,
        CMD_AVSYNC,
        CMD_SOURCE_FILTER_BLOCKED,
        CMD_UDEC_IN_RUNNING_STATE,
        CMD_STEP,
        CMD_CONFIG,
        CMD_AUDIO,
        CMD_ACK,
        CMD_FLOW_CONTROL,
        CMD_DELETE_FILE,
        CMD_REALTIME_SPEEDUP,
        CMD_UPDATE_PLAYBACK_DIRECTION,
        CMD_LAST,
    };

    struct CMD {
        AM_UINT code;
        void    *pExtra;
        AM_U8 repeatType;
        AM_U8 flag;
        AM_U8 needFreePExtra;
        AM_U8 reserved1;
        AM_U32 flag2;
        AM_U32 res32_1;
        AM_U64 res64_1;
        AM_U64 res64_2;

        CMD(int cid) { code = cid; pExtra = NULL; needFreePExtra = 0; repeatType = 0;}
        CMD() {pExtra = NULL; needFreePExtra = 0; repeatType = 0;}
    };

public:
    DECLARE_INTERFACE(IActiveObject, IID_IActiveObject);
    virtual const char *GetName() = 0;
    virtual void OnRun() = 0;
    virtual void OnCmd(CMD& cmd) = 0;
};
//-----------------------------------------------------------------------
//
// IMsgPort
//
//-----------------------------------------------------------------------
class IMsgPort: public IInterface
{
public:
	DECLARE_INTERFACE(IMsgPort, IID_IMsgPort);
	virtual AM_ERR PostMsg(AM_MSG& msg) = 0;
         virtual AM_ERR SendMsg(AM_MSG& msg) = 0;
};

//-----------------------------------------------------------------------
//
// IMsgSink
//
//-----------------------------------------------------------------------
class IMsgSink: public IInterface
{
public:
	DECLARE_INTERFACE(IMsgSink, IID_IMsgSink);
	virtual void MsgProc(AM_MSG& msg) = 0;
};
//-----------------------------------------------------------------------
//
// IBufferPool
//
//-----------------------------------------------------------------------
class IBufferPool: public IInterface
{
public:
	DECLARE_INTERFACE(IBufferPool, IID_IBufferPool);

	virtual const char *GetName() = 0;

	virtual void Enable(bool bEnabled = true) = 0;
	virtual bool AllocBuffer(CBuffer*& pBuffer, AM_UINT size) = 0;
	virtual AM_UINT GetFreeBufferCnt() = 0;
	virtual void SetNotifyOwner(void*) = 0;

	virtual void AddRef(CBuffer *pBuffer) = 0;
	virtual void Release(CBuffer *pBuffer) = 0;

	virtual void AddRef() = 0;
	virtual void Release() = 0;
};
//-----------------------------------------------------------------------
//
// IPin
//
//-----------------------------------------------------------------------
class IPin: public IInterface
{
public:
	DECLARE_INTERFACE(IPin, IID_IPin);

	virtual AM_ERR Connect(IPin *pPeer) = 0;
	virtual void Disconnect() = 0;

	virtual void Receive(CBuffer *pBuffer) = 0;
	virtual void Purge() = 0;
	virtual void Enable(bool bEnable) = 0;

	virtual IPin *GetPeer() = 0;
	virtual IFilter *GetFilter() = 0;

	virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat) = 0;
};

//-----------------------------------------------------------------------
//
// IFilter
//
//-----------------------------------------------------------------------
class IFilter: public IInterface
{
public:
    struct INFO
    {
        AM_UINT     nInput;
        AM_UINT     nOutput;
        AM_UINT     mPriority;
        AM_UINT     mFlags;
        AM_INT      mIndex;
        const char	*pName;
    };

    enum FlowControlType
    {
        FlowControl_pause,
        FlowControl_resume,
        FlowControl_eos,
        FlowControl_finalize_file,
        FlowControl_close_audioHAL,
        FlowControl_reopen_audioHAL,
    };

public:
    DECLARE_INTERFACE(IFilter, IID_IFilter);

    virtual AM_ERR Run() = 0;
    virtual AM_ERR Stop() = 0; // stop active threads, and destroy content, (exit OnRun loop, release resources)
    virtual AM_ERR Start() = 0;

    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual void Flush() = 0; // keep thread alive, purge holded buffers and data

    virtual AM_ERR FlowControl(FlowControlType type) = 0;//for pipeline flow control use

    virtual void SendCmd(AM_UINT cmd) = 0;//generic send cmd port
    virtual void PostMsg(AM_UINT cmd) = 0;

    virtual bool IsMaster() = 0;
    virtual void SetMaster(bool isMaster) = 0;

    virtual void OutputBufferNotify(IBufferPool* buffer_pool) = 0;

    virtual void GetInfo(INFO& info) = 0;
    virtual IPin* GetInputPin(AM_UINT index) = 0;
    virtual IPin* GetOutputPin(AM_UINT index) = 0;

    //for dynamic build graph
    virtual AM_ERR AddOutputPin(AM_UINT& index, AM_UINT type) = 0;
    virtual AM_ERR AddInputPin(AM_UINT& index, AM_UINT type) = 0;

#ifdef AM_DEBUG
    virtual void PrintState() = 0;
#endif
};


//-----------------------------------------------------------------------
//
// IParameters
//
//-----------------------------------------------------------------------
enum
{
    ePriDataType_rawbinary = 0,
    ePriDataType_txt,
};

typedef struct
{
    AM_U32 magic_number;
    AM_U32 data_length;
    AM_U16 type;
    AM_U16 subtype;
    AM_U32 reserved;
} SPriDataHeader;

class IParameters: public IInterface
{
public:

    enum TimeUnit {
        TimeUnitDen_90khz = 90000,
        TimeUnitDen_27mhz = 27000000,
        TimeUnitDen_ms = 1000,
        TimeUnitDen_us = 1000000,
        TimeUnitDen_ns = 1000000000,
        TimeUnitNum_fps29dot97 = 3003,
    };

    enum VideoFrameRate {
        VideoFrameRate_MAX = 1024,
        VideoFrameRate_29dot97 = VideoFrameRate_MAX + 1,
        VideoFrameRate_59dot94,
        VideoFrameRate_23dot96,
        VideoFrameRate_240,
        VideoFrameRate_60,
        VideoFrameRate_30,
        VideoFrameRate_24,
        VideoFrameRate_15,
    };

    enum ProtocolType {
        ProtocolType_Invalid = 0,
        ProtocolType_UDP,
        ProtocolType_TCP,
    };

    enum StreammingServerType {
        StreammingServerType_Invalid = 0,
        StreammingServerType_RTSP,
        StreammingServerType_HTTP,
    };

    enum StreammingServerMode {
        StreammingServerMode_MulticastSetAddr = 0, //live streamming/boardcast
        StreammingServerMode_Unicast, //vod
        StreammingServerMode_MultiCastPickAddr, //join conference
    };

    enum StreammingContentType {
        StreammingContentType_Live = 0,
        StreammingContentType_LocalFile,
        StreammingContentType_Conference,
    };

    enum ContainerType {
        MuxerContainer_Invalid = 0,
        MuxerContainer_AUTO = 1,
        MuxerContainer_MP4,
        MuxerContainer_3GP,
        MuxerContainer_TS,
        MuxerContainer_MOV,
        MuxerContainer_MKV,
        MuxerContainer_AVI,
        MuxerContainer_AMR,
        MuxerContainer_RTSP_LiveStreamming,

        MuxerContainer_TotolNum,
    };

    enum StreamType {
        StreamType_Invalid = 0,
        StreamType_Video,
        StreamType_Audio,
        StreamType_Subtitle,
        StreamType_PrivateData,

        StreamType_TotalNum,
    };

    enum StreamFormat{
        StreamFormat_Invalid = 0,
        StreamFormat_H264,
        StreamFormat_AAC,
        StreamFormat_MP2,
        StreamFormat_AC3,
        StreamFormat_ADPCM,
        StreamFormat_AMR_NB,
        StreamFormat_AMR_WB,
        StreamFormat_PrivateData,
        StreamFormat_AmbaTrickPlayData,
        StreamFormat_GPSInfo,
    };

    enum PixFormat{
        PixFormat_YUV420P = 0,
        PixFormat_NV12,
        PixFormat_RGB565,
    };

    enum DataCategory {
        DataCategory_PrivateData = 0,
        DataCategory_SubTitleString,
        DataCategory_SubTitleRaw,
        DataCategory_VideoES,
        DataCategory_AudioES,
        DataCategory_VideoYUV420p,
        DataCategory_VideoNV12,
        DataCategory_AudioPCM,
    };

    enum PrivateDataType {
        PrivateDataType_GPSInfo = 0,
        PrivateDataType_SensorInfo,
        //todo
    };

    enum EntropyType {
        EntropyType_NOTSet = 0,
        EntropyType_H264_CABAC,
        EntropyType_H264_CAVLC,
    };

    enum AudioSampleFMT{
        SampleFMT_NONE = -1,
        SampleFMT_U8,          ///< unsigned 8 bits
        SampleFMT_S16,         ///< signed 16 bits
        SampleFMT_S32,         ///< signed 32 bits
        SampleFMT_FLT,         ///< float
        SampleFMT_DBL,         ///< double
        SampleFMT_NB           ///< Number of sample formats. DO NOT USE if linking dynamically
    };

    enum MuxerSavingFileStrategy {
        MuxerSavingFileStrategy_AutoSeparateFile = 0,//muxer will auto separate file itself, with more time accuracy
        MuxerSavingFileStrategy_ManuallySeparateFile,//engine/app will management the separeting file
        MuxerSavingFileStrategy_ToTalFile,//not recommanded, cannot store file like .mp4 if storage have not enough space
    };

    enum MuxerSavingCondition {
        MuxerSavingCondition_Invalid = 0,
        MuxerSavingCondition_InputPTS = 1,
        MuxerSavingCondition_CalculatedPTS,
        MuxerSavingCondition_FrameCount,
    };

    enum MuxerAutoFileNaming {
        MuxerAutoFileNaming_ByDateTime = 0,
        MuxerAutoFileNaming_ByNumber,
    };

    enum DecoderFeedingRule {
        DecoderFeedingRule_NotValid = 0,
        DecoderFeedingRule_AllFrames,
        DecoderFeedingRule_RefOnly,
        DecoderFeedingRule_IOnly,
        DecoderFeedingRule_IDROnly,
    };

    enum OSDInputDataFormat {
        OSDInputDataFormat_RGBA = 0,
        OSDInputDataFormat_YUVA_CLUT,
    };

    typedef struct
    {
        AM_UINT pic_width;
        AM_UINT pic_height;
        AM_UINT pic_offset_x;
        AM_UINT pic_offset_y;
        AM_UINT framerate_num;
        AM_UINT framerate_den;
        AM_UINT framerate;
        AM_UINT M, N, IDRInterval;//control P, I, IDR percentage
        AM_UINT sample_aspect_ratio_num;
        AM_UINT sample_aspect_ratio_den;
        AM_UINT bitrate;
        AM_UINT lowdelay;
        EntropyType entropy_type;
    } SVideoParams;

    typedef struct
    {
        AM_UINT sample_rate;
        AudioSampleFMT sample_format;
        AM_UINT channel_number;
        AM_UINT channel_layout;
        AM_UINT frame_size;
        AM_UINT bitrate;
        AM_UINT need_skip_adts_header;
        AM_UINT pts_unit_num;//pts's unit
        AM_UINT pts_unit_den;
    } SAudioParams;

    typedef struct
    {
        //to do
        AM_UINT duration;
    } SSubtitleParams;

    typedef struct
    {
        //to do
        AM_UINT duration;
    } SPriDataParams;

    typedef union
    {
        SVideoParams video;
        SAudioParams audio;
        SSubtitleParams subtitle;
        SPriDataParams pridata;
    } UFormatSpecific;

public:
    DECLARE_INTERFACE(IParameters, IID_IParameters);
    virtual AM_ERR SetParameters(StreamType type, StreamFormat format, UFormatSpecific* param, AM_UINT index = 0) = 0;
};

//some constant
enum {
    InvalidAutoSavingParam = 0,
    DefaultAutoSavingParam = 180,
    //h264 encoder related
    DefaultH264M = 1,
    DefaultH264N = 15,
    DefaultH264IDRInterval = 4,
    //some auto save related
    DefaultMaxFileNumber = 1000,
    DefaultTotalFileNumber = 0, //0, no limit
    //private data related
    DefaultPridataDuration = 90000,//1000 ms

    //main window's dimension
    DefaultMainWindowWidth = 1280,
    DefaultMainWindowHeight = 720,

    //video related
    DefaultMainVideoWidth = 1280,
    DefaultMainVideoHeight = 720,
    DefaultMainVideoOffset_x = 0,
    DefaultMainVideoOffset_y = 0,
    DefaultMainBitrate = 10000000,
    DefaultPreviewCWidth = 320,
    DefaultPreviewCHeight = 240,

    //server
    DefaultRTSPServerPort = 554,
    DefaultRTPServerPortBase = 20022,

    //duplex related
    DefaultDuplexH264M = 1,
    DefaultDuplexH264N = 15,
    DefaultDuplexH264IDRInterval = 2,
    DefaultDuplexMainBitrate = 8000000,
    DefaultDuplexPlaybackVoutIndex = eVoutHDMI,
    DefaultDuplexPreviewVoutIndex = eVoutHDMI,
    DefaultDuplexPreviewEnabled = 1,
    DefaultDuplexPlaybackDisplayEnabled = 1,
    DefaultDuplexPreviewAlpha = 240,
    DefaultDuplexPreviewInPIP = 1,

    DefaultDuplexPreviewLeft = 0,
    DefaultDuplexPreviewTop = 0,
    DefaultDuplexPreviewWidth = 320,
    DefaultDuplexPreviewHeight = 180,

    DefaultDuplexPbDisplayLeft = 0,
    DefaultDuplexPbDisplayTop = 0,
    DefaultDuplexPbDisplayWidth = 1280,
    DefaultDuplexPbDisplayheight = 720,

    //thumbnail related, wqvga?
    DefaultThumbnailWidth = 480,
    DefaultThumbnailHeight = 272,

    //dsp piv related
    DefaultJpegWidth = 320,
    DefaultJpegHeight = 240,

    //pb speed
    InvalidPBSpeedParam = 0xff,

    AllTargetTag = 0xff,
};

#define DInvalidGeneralIntParam 0xffffffff
//some preset save related
#define DDefaultPreSetMaxFileSize 0x7fffffffffffffffLL
#define DDefaultPresetMaxFrameCount  0x7fffffffffffffffLL

//some hardware related enum
enum {
    DSPMode_Invalid = 0,
    DSPMode_Idle,
    DSPMode_CameraRecording,
    DSPMode_CameraPlayback,
    DSPMode_UDEC,
    DSPMode_DuplexLowdelay,
};

//-----------------------------------------------------------------------
//
//IDemuxer
//
//-----------------------------------------------------------------------
class IDemuxer: public IInterface
{
public:
    DECLARE_INTERFACE(IDemuxer, IID_IDemuxer);
    //virtual void Decode() = 0;
    virtual AM_ERR LoadFile(const char *pFileName, void *pParserObj) = 0;
    /*For duplex mode, check whether this file can be played or not
    *     must be called after LoadFile()
    */
    virtual bool isSupportedByDuplex() = 0;
    virtual AM_ERR Seek(AM_U64 &ms) = 0;
    virtual AM_ERR GetTotalLength(AM_U64& ms) = 0;
    virtual AM_ERR GetFileType(const char *&pFileType) = 0;
    virtual void EnableAudio(bool enable) = 0;
    virtual void EnableVideo(bool enable) = 0;
    virtual void EnableSubtitle(bool enable) = 0;
    /*A5S DV Playback Test */
    virtual AM_ERR  A5SPlayMode(AM_UINT mode) = 0;
    virtual AM_ERR  A5SPlayNM(AM_INT start_n, AM_INT end_m) = 0;
    /*end A5S DV Test*/

    virtual void SetIndex(AM_INT index) = 0;
    virtual void GetVideoSize(AM_INT *pWidth, AM_INT *pHeight) = 0;
};

//decoder status
enum {
    DecoderStatus_notInited = 0,
    DecoderStatus_ready,
    DecoderStatus_busyWaitInputBuffer,//alloc bsb?
    DecoderStatus_busyFlushing,
    DecoderStatus_busyHandlingEOS,
    DecoderStatus_busyDecoding,
    DecoderStatus_genericError,
    DecoderStatus_udecError,
};

typedef struct
{
    void* pHandle; //AVStream*?

    void* p_filter;
    void* p_outputpin;
    AM_UINT codec_format;
    //max picture size, maybe not-aligned
    AM_UINT max_width, max_height;

    //extradata
    AM_U8* extradata;
    AM_UINT extradata_size;
    //frame rate
    AM_UINT framerate_num, framerate_den;

} DecoderConfig;

class IDecoder: public IInterface
{
public:
    DECLARE_INTERFACE(IDecoder, IID_IDecoder);
    virtual AM_ERR ConstructContext(DecoderConfig* configData) = 0;
    virtual AM_ERR Decode(CBuffer* pBuffer) = 0;//need handle data/eos
    virtual void Flush() = 0;
    virtual void Stop() = 0;
    virtual AM_ERR FillEOS() = 0;
    virtual AM_ERR IsReady(CBuffer* pBuffer) = 0;
    virtual AM_UINT GetDecoderStatus() = 0;
    virtual void PrintState() = 0;
    //direct rendering
    virtual void SetOutputPin(void* pOutPin) = 0;
    virtual AM_INT GetDecoderInstance() = 0;
    //virtual AM_ERR GetDecodedFrame(CBuffer* pVideoBuffer) = 0;
};

IDecoder* AMCreateDecoder(AM_UINT decoderType, DecoderConfig* configData);
void AMDestroyDecoder(IDecoder* pDecoder);


class IEditor: public IInterface
{
public:
    DECLARE_INTERFACE(IDecoder, IID_IDecoder);
    virtual void Decode() = 0;

    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual void Flush() = 0;
    //pause / resume -->up
    //flush--->up
    //start no need
};

//-----------------------------------------------------------------------
//
// IRenderer
//
//-----------------------------------------------------------------------
class IRender: public IInterface
{
public:
	DECLARE_INTERFACE(IRender, IID_IRenderer);
	//virtual AM_ERR Pause() = 0;
	//v/irtual AM_ERR Resume() = 0;
        // absoluteTimeMs = (lastPTS - firstPTS) + nSampleCount/SampleRate - latency for audio
    //                  (lastPTS - firstPTS) + nFrameCount/FrameRate             for video

    // relativeTimeMs = nSampleCount/SampleRate for audio
    //                  nFrameCount/FrameRate   for video
    virtual AM_ERR GetCurrentTime(AM_U64& absoluteTimeMs, AM_U64& relativeTimeMs) = 0;

};

//-----------------------------------------------------------------------
//
// IDataRetriever
//
//-----------------------------------------------------------------------
typedef void (*data_retriever_callback_t)(void* cookie, AM_U8* pdata, AM_UINT datasize, AM_U16 type, AM_U16 sub_type);
class IDataRetriever: public IInterface
{
public:
    DECLARE_INTERFACE(IDataRetriever, IID_IDataRetriever);
    virtual AM_ERR RetieveData(AM_U8* pdata, AM_UINT max_len, IParameters::DataCategory data_category = IParameters::DataCategory_PrivateData) = 0;
    virtual AM_ERR RetieveDataByType(AM_U8* pdata, AM_UINT max_len, AM_U16 type, AM_U16 sub_type, IParameters::DataCategory data_category = IParameters::DataCategory_PrivateData) = 0;
    virtual AM_ERR SetDataRetrieverCallBack(void* cookie, data_retriever_callback_t callback) = 0;
};

//-----------------------------------------------------------------------
//
// IEngine
//
//-----------------------------------------------------------------------
class IEngine: public IInterface
{
public:
	enum {
		MSG_ERROR,
		MSG_READY,
		MSG_DURATION,
		MSG_FILESIZE,
		MSG_EOS,
		MSG_AVSYNC,
		MSG_SUBERROR,
		MSG_SWITCH_MASTER_RENDER,//for bug1294  parameter: p0={0:audio; 1:video}
		MSG_VIDEO_RESOLUTION_CHANGED,
		MSG_NEWFILE_GENERATED,
		MSG_STORAGE_ERROR,
		MSG_OS_ERROR,//driver error
		MSG_PRIDATA_GENERATED,
		MSG_SOURCE_FILTER_BLOCKED,
		MSG_NOTIFY_UDEC_IS_RUNNING,
		MSG_AUDIO_PTS_LEAPED,
		MSG_VIDEO_DECODING_START,
		MSG_GENERATE_THUMBNAIL,
		MSG_FILE_NUM_REACHED_LIMIT,
		MSG_EVENT_NOTIFICATION,
		MSG_LAST,
	};

public:
	DECLARE_INTERFACE(IEngine, IID_IEngine);
	virtual AM_ERR PostEngineMsg(AM_MSG& msg) = 0;
	virtual void *QueryEngineInterface(AM_REFIID refiid) = 0;
         //virtual CMsgSys* GetMsgSys() = 0;
public:
	// helper
	AM_ERR PostEngineMsg(AM_UINT code)
	{
		AM_MSG msg;
		msg.code = code;
		return PostEngineMsg(msg);
	}
	void* mpOpaque;

};
//-----------------------------------------------------------------------
//
// IVideoOutput
//
//-----------------------------------------------------------------------
class IVideoOutput: public IInterface
{
public:
    DECLARE_INTERFACE(IVideoOutput, IID_IVideoOutput);
    //trick mode
    virtual AM_ERR Step() = 0;
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
    virtual AM_ERR EnableVoutAAR(AM_INT enable) = 0;//enable vout auto aspect ratio
    //source/dest rect and scale mode config
    virtual AM_ERR SetVideoSourceRect(AM_INT x, AM_INT y, AM_INT w, AM_INT h) = 0;
    virtual AM_ERR SetVideoDestRect(AM_INT vout_id, AM_INT x, AM_INT y, AM_INT w, AM_INT h) = 0;
    virtual AM_ERR SetVideoScaleMode(AM_INT vout_id, AM_UINT mode_x, AM_UINT mode_y) = 0;

    virtual AM_ERR SetDeWarpControlWidth(AM_UINT enable, AM_UINT width_top, AM_UINT width_bottom) = 0;
};

//-----------------------------------------------------------------------
//
// IMediaControl
//
//-----------------------------------------------------------------------
class IMediaControl: public IInterface
{
public:
	// msg post to app
	enum {
		MSG_STATE_CHANGED,
		MSG_PLAYER_EOS,
		MSG_RECORDER_EOS,
		MSG_ERROR_POST_TO_APP,
		MSG_STORAGE_ERROR_POST_TO_APP,
		MSG_OS_ERROR_POST_TO_APP,
		MSG_NEWFILE_GENERATED_POST_TO_APP,
		MSG_RECORDER_REACH_DURATION,
		MSG_RECORDER_REACH_FILESIZE,
		MSG_RECORDER_REACH_FILENUM,
		MSG_EVENT_NOTIFICATION_TO_APP,
		MSG_EVENT_NVR_MOTION_DETECT_NOTIFY,
	};

public:
	DECLARE_INTERFACE(IMediaControl, IID_IMediaControl);
	virtual AM_ERR SetAppMsgSink(IMsgSink *pAppMsgSink) = 0;
	virtual AM_ERR SetAppMsgCallback(void (*MsgProc)(void*, AM_MSG&), void *context) = 0;
};

//-----------------------------------------------------------------------
//
// IMediaControlOnTheFly
//
//-----------------------------------------------------------------------
class IMediaControlOnTheFly: public IInterface
{
public:
    DECLARE_INTERFACE(IMediaControlOnTheFly, IID_IMediaControlOnTheFly);

    //display related
    virtual AM_ERR UpdatePBDisplay(AM_UINT size_x, AM_UINT size_y, AM_UINT pos_x, AM_UINT pos_y) = 0;
    virtual AM_ERR UpdatePreviewDisplay(AM_UINT size_x, AM_UINT size_y, AM_UINT pos_x, AM_UINT pos_y, AM_UINT alpha) = 0;
    virtual AM_ERR GetPBDisplayProperty(AM_UINT& vout_id, AM_UINT& in_pip, AM_UINT& size_x, AM_UINT& size_y) = 0;
    virtual AM_ERR GetPreviewProperty(AM_UINT& vout_id, AM_UINT& in_pip, AM_UINT& size_x, AM_UINT& size_y, AM_UINT& alpha) = 0;

    //encoding releated
    virtual AM_ERR DemandIDR(AM_UINT method, AM_U64 pts) = 0;
    virtual AM_ERR UpdateEncGop(AM_UINT M, AM_UINT N, AM_UINT idr_interval, AM_UINT gop_structure) = 0;
    virtual AM_ERR UpdateEncBitrate(AM_UINT bitrate) = 0;
    virtual AM_ERR UpdateEncFramerate(AM_UINT reduce_factor, AM_UINT framerate_integar) = 0;
};

//-----------------------------------------------------------------------
//
// IStreamingControl
//
//-----------------------------------------------------------------------
class IStreamingControl: public IInterface
{
public:
    DECLARE_INTERFACE(IStreamingControl, IID_IStreamingControl);
    virtual AM_ERR AddStreamingServer(AM_UINT& server_index, IParameters::StreammingServerType type, IParameters::StreammingServerMode mode) = 0;
    virtual AM_ERR RemoveStreamingServer(AM_UINT server_index) = 0;
    virtual AM_ERR EnableStreamming(AM_UINT server_index, AM_UINT output_index, AM_UINT streaming_index) = 0;
    virtual AM_ERR DisableStreamming(AM_UINT server_index, AM_UINT output_index, AM_UINT streaming_index) = 0;
    virtual AM_ERR GetStreamingPortUrl(AM_UINT server_index, AM_UINT &server_port, AM_S8 *url, AM_UINT url_size) = 0;
};

//-----------------------------------------------------------------------
//
// ISimplePlayback
//
//-----------------------------------------------------------------------
class ISimplePlayback: public IInterface
{
public:
    DECLARE_INTERFACE(ISimplePlayback, IID_ISimplePlayback);
    virtual AM_ERR PlayFile(const char *pFileName) = 0;
    virtual AM_ERR StopPlay() = 0;
    virtual AM_ERR PausePlay() = 0;
    virtual AM_ERR ResumePlay() = 0;
    virtual AM_ERR SeekPlay(AM_U64 time) = 0;

    virtual void SetAudioSink(void* audio_sink) = 0;
    virtual AM_ERR SetPBProperty(AM_UINT prop, AM_UINT value) = 0;
};

//-----------------------------------------------------------------------
//
// IClockSource
//
//-----------------------------------------------------------------------
class IClockSource: public IInterface
{
public:
	typedef enum {
	            STOPPED,
	            RUNNING,
	            PAUSED
	} CLOCK_STATE;

public:
    DECLARE_INTERFACE(IClockSource, IID_IClockSource);
    virtual am_pts_t GetClockTime() = 0;
    virtual am_pts_t GetClockBase() = 0;
    virtual void SetClockUnit(AM_UINT num, AM_UINT den) = 0;
    virtual void GetClockUnit(AM_UINT& num, AM_UINT& den) = 0;

    //am_pts_t GetClockStartPts() = 0;
    virtual void SetClockState(CLOCK_STATE state) = 0;
    //virtual void SetClockStartPts(am_pts_t pts) = 0;
};

//-----------------------------------------------------------------------
//
// IClockObserver
//
//-----------------------------------------------------------------------
class IClockObserver: public IInterface
{
public:
	DECLARE_INTERFACE(IClockObserver, IID_IClockObserver);
	virtual AM_ERR OnTimer(am_pts_t curr_pts) = 0;
};

//-----------------------------------------------------------------------
//
// IClockManager
//
//-----------------------------------------------------------------------
class IClockManager: public IInterface
{
public:
	DECLARE_INTERFACE(IClockManager, IID_IClockManager);
	virtual AM_ERR SetTimer(IClockObserver *pObserver, am_pts_t pts) = 0;
	virtual AM_ERR DeleteTimer(IClockObserver *pObserver) = 0;
	virtual am_pts_t GetCurrentTime() = 0;
	virtual AM_ERR StartClock() = 0;
	virtual void StopClock() = 0;
	virtual void PauseClock() = 0;
	virtual void ResumeClock() = 0;
	virtual void SetSource(IClockSource *pSource) = 0;
	virtual void SetStartTime(am_pts_t time) = 0;
	virtual am_pts_t GetStartTime() = 0;
	virtual void SetStartPts(am_pts_t time) = 0;
	virtual am_pts_t GetStartPts() = 0;
	virtual void PurgeClock() = 0;//clear all remaining timers, do it only when flush
	virtual void SetClockMgr(am_pts_t startPts) = 0;
};
//-----------------------------------------------------------------------
//
// IFileReader
//
//-----------------------------------------------------------------------
class IFileReader: public IInterface
{
public:
	DECLARE_INTERFACE(IFileReader, IID_IFileReader);
	virtual AM_ERR OpenFile(const char *pFileName) = 0;
	virtual AM_ERR CloseFile() = 0;
	virtual am_file_off_t GetFileSize() = 0;
	virtual AM_ERR ReadFile(am_file_off_t offset, void *pBuffer, AM_UINT size) = 0;
};

//-----------------------------------------------------------------------
//
// IDecoderControl
//
//-----------------------------------------------------------------------
class IDecoderControl: public IInterface
{
public:
    DECLARE_INTERFACE(IDecoderControl, IID_IDecoderControl);
    virtual AM_ERR ReConfigDecoder(AM_INT flag) = 0;
    virtual AM_ERR AudioPtsLeaped() = 0;
};

//-----------------------------------------------------------------------
//
// IWatcher
//
//-----------------------------------------------------------------------
enum {
    WATCH_TYPE_SOCKET = 0,
    WATCH_TYPE_DIR,
    WATCH_TYPE_NOTIFY_MSG,
};

typedef struct {
    AM_INT export_fd;
    AM_U8 export_type;

    AM_U8 reserved0[3];
} SWatchItem;

typedef AM_ERR (*TFWatchCallbackNotify) (void* thiz, SWatchItem* owner, unsigned int flag);

class IWatcher: public IInterface
{
public:
    DECLARE_INTERFACE(IDecoderControl, IID_IWatcher);
    virtual SWatchItem* AddWatchItem(char* name, AM_UINT watch_mask, AM_INT fd, AM_U8 type) = 0;
    virtual AM_ERR RemoveWatchItem(SWatchItem* content) = 0;

    virtual AM_ERR StartWatching(void* thiz, TFWatchCallbackNotify notify_cb) = 0;
    virtual AM_ERR StopWatching() = 0;
};

enum {
    AM_MSG_TYPE_INVALID = 0x00,
    AM_MSG_TYPE_NEW_RTSP_URL,
    AM_MSG_TYPE_MOTION_DETECTED,
    AM_MSG_TYPE_NO_MOTION_DETECTED,
    AM_MSG_TYPE_IDR_NOTIFICATION,
};

typedef AM_INT (*TFConfigUpdateNotify) (void* thiz, AM_UINT msg_type, void* msg_body);

//-----------------------------------------------------------------------
//
// callback prototype
//
//-----------------------------------------------------------------------
typedef AM_ERR (*AM_CallbackGetUnPackedPriData)(void* p_content, AM_U8* & p_data, AM_UINT& len, AM_U16& data_type, AM_U16& sub_type);
typedef AM_ERR (*AM_CallbackGetPackedPriData)(void* p_content, AM_U8* & p_data);


//-----------------------------------------------------------------------
//
// global APIs
//
//-----------------------------------------------------------------------
extern AM_ERR AMF_Init();
extern void AMF_Terminate();

#endif
