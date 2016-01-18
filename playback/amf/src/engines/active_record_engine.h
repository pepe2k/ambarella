
/**
 * active_pb_engine.h
 *
 * History:
 *    2011/07/11 - [Jay Zhang] created file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __ACTIVE_RECORD_ENGINE_H__
#define __ACTIVE_RECORD_ENGINE_H__

//-----------------------------------------------------------------------
//
// CRecordEngine2
//
//-----------------------------------------------------------------------

class CRecordEngine2:
    public CActiveEngine,
    public IRecordControl2,
    public IStreammingContentProvider,
    public IStreamingControl
{
    typedef CActiveEngine inherited;

    typedef struct
    {
        IParameters::StreamType stream_type;
        IParameters::StreamFormat stream_format;
        IParameters::UFormatSpecific spec;
    } SStreamInfo;

    typedef struct
    {
        SStreamInfo stream_info[DMaxStreamNumber];
        AM_UINT stream_number;
        IParameters::ContainerType out_format;

        AM_UINT video_number;
        AM_UINT audio_number;
        AM_UINT subtitle_number;
        AM_UINT pridata_number;

        AM_UINT video_streamming;
        AM_UINT audio_streamming;

        AM_U64 mMaxFileDurationUs;
        AM_U64 mMaxFileSizeBytes;

        //auto cut related parameters
        IParameters::MuxerSavingFileStrategy mSavingStrategy;
        IParameters::MuxerSavingCondition mSavingCondition;
        IParameters::MuxerAutoFileNaming mAutoNaming;
        AM_UINT mConditionParam;
        AM_UINT mMaxFileNumber;
        AM_UINT mTotalFileNumber;
    } SMuxerConfig;

    typedef struct _SPrivateDataConfig
    {
        struct _SPrivateDataConfig* p_next;
        AM_UINT duration;
        AM_U16  data_type;
        AM_U16 reserved;
    } SPrivateDataConfig;

    //record defined cmd/msg
    enum {
        REC_FIRST_CMD = IActiveObject::CMD_LAST,
        RECCMD_STOP,
        RECCMD_ABORT,
        RECCMD_PAUSE_RESUME,
        RECCMD_CLOSE_OPEN_AUDIO_HAL,
        REC_LAST_CMD,
    };

public:
    static CRecordEngine2* Create(int streaming_only = 0);

private:
    CRecordEngine2(int streaming_only = 0);
    AM_ERR Construct();
    virtual ~CRecordEngine2();

public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete();

    // IEngine
    virtual AM_ERR PostEngineMsg(AM_MSG& msg)
    {
        return inherited::PostEngineMsg(msg);
    }

    virtual void *QueryEngineInterface(AM_REFIID refiid);

    // IMediaControl
    virtual AM_ERR SetAppMsgSink(IMsgSink *pAppMsgSink)
    {
        return inherited::SetAppMsgSink(pAppMsgSink);
    }
    virtual AM_ERR SetAppMsgCallback(void (*MsgProc)(void*, AM_MSG&), void *context)
    {
        return inherited::SetAppMsgCallback(MsgProc, context);
    }

    // IRecordControl2
    virtual AM_ERR SetWorkingMode(AM_UINT dspMode, AM_UINT voutMask)
    {
        AMLOG_INFO("rec engine SetWorkingMode mode %d, voutmask %d.\n", dspMode, voutMask);
        if (DSPMode_CameraRecording == dspMode) {
            AMLOG_INFO("SetWorkingMode dspMode DSPMode_CameraRecording.\n");
            mShared.encoding_mode_config.dsp_mode = DSPMode_CameraRecording;
        } else if (DSPMode_DuplexLowdelay == dspMode) {
            AMLOG_INFO("SetWorkingMode dspMode DSPMode_DuplexLowdelay.\n");
            mShared.encoding_mode_config.dsp_mode = DSPMode_DuplexLowdelay;
        } else {
            AM_ERROR("BAD dspMode %d in SetWorkingMode.\n", dspMode);
            return ME_BAD_PARAM;
        }
        mRequestVoutMask = voutMask;
        return ME_OK;
    }

    virtual AM_ERR GetRecInfo(INFO& info)
    {
        info.state = mState;
        return ME_OK;
    }
    virtual AM_ERR SetPrivateDataDuration(AM_UINT duration, AM_U16 data_type);

    virtual void PrintState();

    virtual AM_ERR SetTotalOutputNumber(AM_UINT& tot_number);
    virtual AM_ERR SetupOutput(AM_UINT out_index, const char *pFileName, IParameters::ContainerType out_format);
    virtual AM_ERR SetupThumbNailFile(const char * pThumbNailFileName);
    virtual AM_ERR SetupThumbNailParam(AM_UINT out_index,AM_INT enabled, AM_INT width, AM_INT height);
    virtual AM_ERR SetSavingStrategy(IParameters::MuxerSavingFileStrategy strategy, IParameters::MuxerSavingCondition condition = IParameters::MuxerSavingCondition_InputPTS, IParameters::MuxerAutoFileNaming naming = IParameters::MuxerAutoFileNaming_ByNumber, AM_UINT param = DefaultAutoSavingParam, AM_UINT channel = DInvalidGeneralIntParam);
    virtual AM_ERR SetMaxFileNumber(AM_UINT max_file_num, AM_UINT channel = DInvalidGeneralIntParam);
    virtual AM_ERR SetTotalFileNumber(AM_UINT total_file_num, AM_UINT channel = DInvalidGeneralIntParam);
    virtual AM_ERR NewStream(AM_UINT out_index, AM_UINT& stream_index, IParameters::StreamType type, IParameters::StreamFormat format);
    virtual AM_ERR StartRecord();
    virtual AM_ERR StopRecord();
    virtual void AbortRecord();

    virtual AM_ERR PauseRecord();
    virtual AM_ERR ResumeRecord();

    virtual AM_ERR CloseAudioHAL();
    virtual AM_ERR ReopenAudioHAL();

    virtual AM_ERR ExitPreview();//should be called after stop encoding
    virtual AM_ERR SetProperty(AM_UINT prop, AM_UINT value);

    virtual AM_ERR DisableVideo();
    virtual AM_ERR DisableAudio();

    //video parameters (optional) api:
    virtual AM_ERR SetVideoStreamDimention(AM_UINT out_index, AM_UINT stream_index, AM_UINT width, AM_UINT height);
    virtual AM_ERR SetVideoStreamFramerate(AM_UINT out_index, AM_UINT stream_index, AM_UINT framerate_num, AM_UINT framerate_den);
    virtual AM_ERR SetVideoStreamBitrate(AM_UINT out_index, AM_UINT stream_index, AM_UINT bitrate);
    virtual AM_ERR SetVideoStreamLowdelay(AM_UINT out_index, AM_UINT stream_index, AM_UINT lowdelay);
    virtual AM_ERR SetVideoStreamEntropyType(AM_UINT out_index, AM_UINT stream_index, IParameters::EntropyType entropy_type);
    virtual AM_ERR DemandIDR(AM_UINT out_index);
    virtual AM_ERR UpdateGOPStructure(AM_UINT out_index, AM_INT M, AM_INT N, AM_INT idr_interval);
    virtual AM_ERR UpdatePreviewDisplay(AM_UINT width, AM_UINT height, AM_UINT pos_x, AM_UINT pos_y, AM_UINT alpha);

    virtual AM_ERR SetDisplayMode(AM_UINT display_preview, AM_UINT display_playback, AM_UINT preview_vout, AM_UINT pb_vout);
    virtual AM_ERR GetDisplayMode(AM_UINT& display_preview, AM_UINT& display_playback, AM_UINT& preview_vout, AM_UINT& pb_vout);

    //audio parameters (optional) api:
    virtual AM_ERR SetAudioStreamChannelNumber(AM_UINT out_index, AM_UINT stream_index, AM_UINT channel);
    virtual AM_ERR SetAudioStreamSampleFormat(AM_UINT out_index, AM_UINT stream_index, IParameters::AudioSampleFMT sample_fmt);
    virtual AM_ERR SetAudioStreamSampleRate(AM_UINT out_index, AM_UINT stream_index, AM_UINT samplerate);
    virtual AM_ERR SetAudioStreamBitrate(AM_UINT out_index, AM_UINT stream_index, AM_UINT bitrate);

    //subtitle parameters (optional) api: todo

    //max-duration, max-filesize
    virtual AM_ERR SetMaxFileDuration(AM_UINT out_index,AM_U64 timeUs) ;
    virtual AM_ERR SetMaxFileSize(AM_UINT out_index,AM_U64 bytes);

    //pravate data api:
    virtual AM_ERR AddPrivateDataPacket(AM_U8* data_ptr, AM_UINT len, AM_U16 data_type, AM_U16 sub_type);

    //IStreammingContentProvider
    virtual AM_UINT GetStreamContentNumber();
    virtual SStreamContext* GetStreamContent(AM_UINT index);
    virtual IStreammingDataTransmiter* GetDataTransmiter(SStreamContext*p_content, AM_UINT index);

    //IStreamingControl
    virtual AM_ERR AddStreamingServer(AM_UINT& server_index, IParameters::StreammingServerType type, IParameters::StreammingServerMode mode);
    virtual AM_ERR RemoveStreamingServer(AM_UINT server_index);
    virtual AM_ERR EnableStreamming(AM_UINT server_index, AM_UINT output_index, AM_UINT streaming_index);
    virtual AM_ERR DisableStreamming(AM_UINT server_index, AM_UINT output_index, AM_UINT streaming_index);
    virtual AM_ERR GetStreamingPortUrl(AM_UINT server_index, AM_UINT &server_port, AM_S8 *url, AM_UINT url_size);

    virtual AM_INT AddOsdBlendArea(AM_INT xPos, AM_INT yPos, AM_INT width, AM_INT height);
    virtual AM_ERR UpdateOsdBlendArea(AM_INT index, AM_U8* data, AM_INT width, AM_INT height);
    virtual AM_ERR RemoveOsdBlendArea(AM_INT index);

    virtual AM_INT AddOsdBlendAreaCLUT(AM_INT xPos, AM_INT yPos, AM_INT width, AM_INT height);
    virtual AM_ERR UpdateOsdBlendAreaCLUT(AM_INT index, AM_U8* data, AM_U32* p_input_clut, AM_INT width, AM_INT height, IParameters::OSDInputDataFormat data_format);
    virtual AM_ERR RemoveOsdBlendAreaCLUT(AM_INT index);

    virtual AM_ERR FreezeResumeHDMIPreview(AM_INT flag);

    //raw data capture
    virtual AM_ERR CaptureYUV(char* name, SYUVData* yuv = NULL);

    virtual AM_ERR CaptureJPEG(const char* name);

private:
    // IActiveObject
    void OnRun();
    AM_UINT allMuxerEOS();
    void clearAllEOS();
    void setMuxerEOS(IFilter* pfilter);
    AM_ERR defaultVideoParameters(AM_UINT out_index, AM_UINT stream_index, AM_UINT ucode_stream_index);
    AM_ERR defaultAudioParameters(AM_UINT out_index, AM_UINT stream_index);
    void checkDupexParameters(void);
    AM_ERR checkParameters(void);
    AM_ERR setOutputFile(AM_UINT index);
    AM_ERR setThumbNailFile(AM_UINT index);
    //AM_ERR findFirstVideoStreamInMuxer(AM_UINT muxer_index, AM_UINT& video_index);
    AM_ERR findNextStreamIndex(AM_UINT muxer_index, AM_UINT& index, IParameters::StreamType type);
    SStreamContext* findStreamingContentByIndex(AM_UINT output_index);
    SStreamContext* generateStreamingContent(AM_UINT output_index);
    SStreammingServerInfo* findStreamingServerByIndex(AM_UINT server_index);

    AM_ERR getParametersFromRecConfig();
    void SetState(IRecordControl2::STATE state)
    {
        AMLOG_INFO("Enter state %d\n", state);
        IRecordControl2::STATE ostate = mState;
        mState = state;
        if (ostate != mState) {
            PostAppMsg(IMediaControl :: MSG_STATE_CHANGED);
        }
    }
    void sendStopRecord();
    bool allvideoDisabled();
    bool allaudioDisabled();
    bool allpridataDisabled();
    bool allstreamingDisabled();
    SPrivateDataConfig* findConfigFromPrivateDataType(AM_U16 data_type);
    void setupStreamingServerManger();

private:
    AM_ERR CreateGraph(void);
    AM_ERR CreateGraph_muxer2(void);
#if PLATFORM_ANDROID_ITRON
    AM_ERR CreateGraph_ITron(void);
#endif
    bool ProcessGenericMsg(AM_MSG& msg);
    bool ProcessGenericCmd(CMD& cmd);
    void ProcessCMD(CMD& oricmd);

private:
    IFilter *mpVideoEncoderFilter;
    IFilter *mpAudioInputFilter;
    IFilter *mpAudioEncoderFilter;
    IFilter *mpPridataComposerFilter;

    IVideoEncoder *mpVideoEncoder;
    IAudioInput *mpAudioInput;
    IAudioEncoder *mpAudioEncoder;
    IPridataComposer* mpPridataComposer;
    IClockManager *mpClockManager;

    //more than one muxer case
    IFilter *mpMuxerFilters[DMaxMuxerNumber];
    //need add the api to IFilter?
    IMuxerControl *mpMuxerControl[DMaxMuxerNumber];
    IStreammingDataTransmiter* mpStreamingDataTransmiter[DMaxMuxerNumber];
    AM_UINT mTotalMuxerNumber;
    AM_UINT mbEOSComes[DMaxMuxerNumber];
    SMuxerConfig mMuxerConfig[DMaxMuxerNumber];
    char* mFilename[DMaxMuxerNumber];

    STATE mState;
    bool mbRun;
    bool mbNeedCreateGragh;
    bool mbStopCmdSent;

    CMutex *mpMutex;
    CCondition *mpCondStopfilters;

private:
    //platform related:
    AM_UINT mTotalVideoStreamNumber;
    //IParameters::StreamFormat mVideoFormat;
    //AM_UINT mMaxFileNumber;
    //AM_UINT mPridataDuration;

private:
    //IParameters::MuxerSavingFileStrategy mSavingFileStrategy;
    //IParameters::MuxerSavingCondition mSavingCondition;
    //AM_UINT mAutoSavingParam;
private:
    SConsistentConfig mShared;

private:
    SPrivateDataConfig* mpPrivateDataConfigList;

private:
    bool mbAllVideoDisabled;
    bool mbAllAudioDisabled;
    bool mbAllPridataDisabled;
    bool mbAllStreamingDisabled;

private:
    IStreammingServerManager* mpStreammingServerManager;
    AM_UINT mnStreammingServerNumber;
    AM_UINT mnStreammingContentNumber;
    CDoubleLinkedList mStreammingServerList;
    CDoubleLinkedList mStreamContextList;

    AM_UINT mServerMagicNumber;//identify server

private:
    AM_UINT mRequestVoutMask;

//thumbnail
private:
    char* mpThumbNailFilename;//chuchen, 2012_5_18
    AM_U8* mpThumbNailRawBuffer;
    AM_UINT mThumbNailRawBufferSize;
    AM_UINT mThumbNailWidth;
    AM_UINT mThumbNailHeight;

private:
    bool mbOnlyEncAudio;
    bool mbOnlyEncVideo;
};


#endif

