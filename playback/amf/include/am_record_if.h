
/**
 * am_record_if.h
 *
 * History:
 *    2010/1/6 - [Oliver Li] created file
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_RECORD_IF_H__
#define __AM_RECORD_IF_H__

extern const AM_IID IID_IRecordControl;
extern const AM_IID IID_IRecordControl2;
extern const AM_IID IID_IAndroidMuxer;
extern const AM_IID IID_IMp4Muxer;
extern const AM_IID IID_IAndroidAudioEncoder;
extern const AM_IID IID_IAndroidRecordControl;
extern const AM_IID IID_IAmbaAuthorControl;

class ICamera;
class AM_IAV;


// {EAB0B161-4CD9-43e1-9414-3F43DA0512A1}
//AM_DEFINE_IID(IID_IAndroidMuxer,
//0xeab0b161, 0x4cd9, 0x43e1, 0x94, 0x14, 0x3f, 0x43, 0xda, 0x5, 0x1f, 0xaf);

//-----------------------------------------------------------------------
//
// IAndroidMuxer
//
//-----------------------------------------------------------------------
class IAndroidMuxer: public  IInterface
{
public:
	typedef enum { //this enum is sync with Andriod MediaRecorder.h now
		OUTPUT_DEF= 0,
		OUTPUT_3GP = 1,
		OUTPUT_MP4 = 2,
		OUTPUT_FORMAT_AUDIO_ONLY_START = 3, // Used in validating the output format.  Should be the
											//  at the start of the audio only output formats.
		/* These are audio only file formats */
		OUTPUT_FORMAT_RAW_AMR = 3, //to be backward compatible
		OUTPUT_FORMAT_AMR_NB = 3,
		OUTPUT_FORMAT_AMR_WB = 4,
		OUTPUT_FORMAT_AAC_ADIF = 5,
		OUTPUT_FORMAT_AAC_ADTS = 6,
		OUTPUT_FORMAT_LIST_END // must be last - used to validate format type
	} OUTPUT_FORMAT;

	typedef enum {//this enum is sync with Andriod MediaRecorder.h now
		AUDIO_ENCODER_DEFAULT = 0,
		AUDIO_ENCODER_AMR_NB = 1,
		AUDIO_ENCODER_AMR_WB = 2,
		AUDIO_ENCODER_AAC = 3,
		AUDIO_ENCODER_AAC_PLUS = 4,
		AUDIO_ENCODER_EAAC_PLUS = 5,
		AUDIO_ENCODER_AC3 = 6,
		AUDIO_ENCODER_MP2 = 7,
		AUDIO_ENCODER_LIST_END // must be the last - used to validate the audio encoder type
	}AUDIO_ENCODER;

public:
	DECLARE_INTERFACE(IAndroidMuxer, IID_IAndroidMuxer);
	virtual AM_ERR SetVideoSize( AM_UINT width, AM_UINT height ) = 0;
	virtual AM_ERR SetVideoFrameRate( AM_UINT framerate ) = 0;
	virtual AM_ERR SetOutputFormat( OUTPUT_FORMAT of ) = 0;
	virtual AM_ERR SetOutputFile(const char *pFileName) = 0;
	virtual AM_ERR SetAudioEncoder(AUDIO_ENCODER ae) = 0;
	virtual AM_ERR SetAudioParameters(AM_UINT audioSource, AM_UINT samplingRate, AM_UINT numberOfChannels, AM_UINT bitrate) = 0;
	virtual AM_ERR SetMaxDuration(AM_U64 limit) = 0;
	virtual AM_ERR SetMaxFileSize(AM_U64 limit) = 0;
};

//-----------------------------------------------------------------------
//
// IMp4Muxer
//
//-----------------------------------------------------------------------
struct CMP4MUX_H264_INFO {
	AM_U16 width;
	AM_U16 height;
	AM_U32 fps;
};

struct CMP4MUX_AUDIO_INFO {
	AM_U32 sample_freq;
	AM_U32 chunk_size;
	AM_U16 sample_size;
	AM_U16 channels;
};

struct CMP4MUX_RECORD_INFO {
	AM_U32 max_filesize;
	AM_U32 max_videocnt;
};

struct CMP4MUX_CONFIG{
	char*	dest_name;
	CMP4MUX_H264_INFO* h264_info;
	CMP4MUX_AUDIO_INFO* audio_info;
	CMP4MUX_RECORD_INFO* record_info;
};

class IMp4Muxer: public IInterface
{
public:
	DECLARE_INTERFACE(IMp4Muxer, IID_IMp4Muxer);
	virtual AM_ERR SetOutputFile (const char *pFileName) = 0;
	virtual AM_ERR Config(CMP4MUX_CONFIG* pConfig) = 0;
};

//-----------------------------------------------------------------------
//
// IAndroidAudioEncoder
//
//-----------------------------------------------------------------------
class IAndroidAudioEncoder: public  IInterface
{
public:
	typedef enum {
		SA_NONE,
		SA_STOP,
		SA_ABORT
	} STOP_ACTION;

	typedef enum {//this enum is sync with Andriod MediaRecorder.h now
		AUDIO_ENCODER_DEFAULT = 0,
		AUDIO_ENCODER_AMR_NB = 1,
		AUDIO_ENCODER_AMR_WB = 2,
		AUDIO_ENCODER_AAC = 3,
		AUDIO_ENCODER_AAC_PLUS = 4,
		AUDIO_ENCODER_EAAC_PLUS = 5,
		AUDIO_ENCODER_AC3 = 6,
		AUDIO_ENCODER_MP2 = 7,
		AUDIO_ENCODER_LIST_END // must be the last - used to validate the audio encoder type
	}AUDIO_ENCODER;

public:
	DECLARE_INTERFACE(IAndroidAudioEncoder, IID_IAndroidAudioEncoder);
	virtual AM_ERR StopEncoding() = 0;
	virtual AM_ERR SetAudioParameters(AM_UINT audioSource, AM_UINT samplingRate, AM_UINT numberOfChannels, AM_UINT bitrate) = 0;
	virtual AM_ERR SetAudioEncoder(AUDIO_ENCODER ae) = 0;
};


//-----------------------------------------------------------------------
//
// IAndroidRecordControl
//
//-----------------------------------------------------------------------
class IAndroidRecordControl: public IMediaControl
{
public:
	typedef enum {
		VIDEO_AVC,
	} VIDEO_FORMAT;

	typedef enum { //this enum is sync with Andriod MediaRecorder.h now
		OUTPUT_DEF= 0,
		OUTPUT_3GP = 1,
		OUTPUT_MP4 = 2,
		OUTPUT_FORMAT_AUDIO_ONLY_START = 3, // Used in validating the output format.  Should be the
											//  at the start of the audio only output formats.
		/* These are audio only file formats */
		OUTPUT_FORMAT_RAW_AMR = 3, //to be backward compatible
		OUTPUT_FORMAT_AMR_NB = 3,
		OUTPUT_FORMAT_AMR_WB = 4,
		OUTPUT_FORMAT_AAC_ADIF = 5,
		OUTPUT_FORMAT_AAC_ADTS = 6,
		OUTPUT_FORMAT_LIST_END // must be last - used to validate format type
	} OUTPUT_FORMAT;

	typedef enum {//this enum is sync with Andriod MediaRecorder.h now
		AUDIO_ENCODER_DEFAULT = 0,
		AUDIO_ENCODER_AMR_NB = 1,
		AUDIO_ENCODER_AMR_WB = 2,
		AUDIO_ENCODER_AAC = 3,
		AUDIO_ENCODER_AAC_PLUS = 4,
		AUDIO_ENCODER_EAAC_PLUS = 5,
		AUDIO_ENCODER_AC3 = 6,
		AUDIO_ENCODER_MP2 = 7,
		AUDIO_ENCODER_LIST_END // must be the last - used to validate the audio encoder type
	}AUDIO_ENCODER;

	typedef enum {
		STATE_STOPPED,
		STATE_RECORDING,
		STATE_STOPPING,
	} STATE;

	struct INFO {
		STATE	state;

	};

public:
	DECLARE_INTERFACE(IAndroidRecordControl, IID_IAndroidRecordControl);

	virtual AM_ERR SetCamera(ICamera *pCamera) = 0;
	//virtual AM_ERR SetIAV(AM_IAV *pIAV) = 0;
	virtual AM_ERR SetVideoFormat( VIDEO_FORMAT format) = 0;
	virtual AM_ERR SetOutputFormat(OUTPUT_FORMAT format) = 0;
	virtual AM_ERR SetVideoSize(AM_UINT width, AM_UINT height) = 0;
	virtual AM_ERR SetVideoFrameRate(AM_UINT framerate) = 0;
	virtual AM_ERR SetVideoQuality(AM_UINT quality) = 0;
	virtual AM_ERR SetAudioEncoder(AUDIO_ENCODER ae) = 0;
	virtual AM_ERR SetAudioParameters(AM_UINT audioSource, AM_UINT samplingRate, AM_UINT numberOfChannels, AM_UINT bitrate) = 0;
	virtual AM_ERR SetMaxDuration(AM_U64 limit) = 0;
	virtual AM_ERR SetMaxFileSize(AM_U64 limit) = 0;
	virtual AM_ERR SetOutputFile(const char *pFileName) = 0;
	virtual AM_ERR StartRecord() = 0;
	virtual AM_ERR StopRecord() = 0;
	virtual void AbortRecord() = 0;
};

extern IAndroidRecordControl* CreateAndroidAuthorControl();
extern IAndroidRecordControl* CreateAudioRecControl();


//-----------------------------------------------------------------------
//
// IRecordControl
//
//-----------------------------------------------------------------------
class IRecordControl: public IMediaControl
{
public:
	typedef enum {
		VIDEO_AVC,
	} VIDEO_FORMAT;

	typedef enum {
		OUTPUT_MP4,
	} OUTPUT_FORMAT;

	typedef enum {
		STATE_STOPPED,
		STATE_RECORDING,
		STATE_STOPPING,
	} STATE;

	struct INFO {
		STATE	state;
	};

public:
	DECLARE_INTERFACE(IRecordControl, IID_IRecordControl);

	virtual AM_ERR SetCamera(ICamera *pCamera) = 0;
	//virtual AM_ERR SetIAV(AM_IAV *pIAV) = 0;
	virtual AM_ERR SetVideoFormat(VIDEO_FORMAT format) = 0;
	virtual AM_ERR SetOutputFormat(OUTPUT_FORMAT format) = 0;

	virtual AM_ERR SetOutputFile(const char *pFileName) = 0;
	virtual AM_ERR StartRecord() = 0;
	virtual AM_ERR StopRecord() = 0;
	virtual void AbortRecord() = 0;
};

//-----------------------------------------------------------------------
//
// IRecordControl2
//
//-----------------------------------------------------------------------
#define DMaxMuxerNumber 2
#define DMaxStreamNumber 8
class IRecordControl2: public IMediaControl
{
public:

    typedef enum {
        STATE_NOT_INITED,//have not build graph, app can set graph related params
        STATE_STOPPED,
        STATE_RECORDING,
        STATE_STOPPING,
    } STATE;

    enum {
        REC_PROPERTY_NOT_START_ENCODING,
        REC_PROPERTY_CAPTURE_JPEG,
        REC_PROPERTY_SELECT_CAMERA,
        REC_PROPERTY_STREAM_ONLY_AUDIO,
        REC_PROPERTY_STREAM_ONLY_VIDEO,
    };

    struct INFO {
        STATE   state;
    };

public:
    DECLARE_INTERFACE(IRecordControl, IID_IRecordControl2);

    virtual AM_ERR SetWorkingMode(AM_UINT dspMode, AM_UINT voutMask) = 0;

    virtual AM_ERR GetRecInfo(INFO& info) = 0;

    virtual AM_ERR SetTotalOutputNumber(AM_UINT& tot_number) = 0;
    virtual AM_ERR SetupOutput(AM_UINT out_index, const char *pFileName, IParameters::ContainerType out_format) = 0;
    virtual AM_ERR SetupThumbNailFile(const char * pThumbNailFileName) = 0;
    virtual AM_ERR SetupThumbNailParam(AM_UINT out_index,AM_INT enabled, AM_INT width, AM_INT height) = 0;
    virtual AM_ERR SetSavingStrategy(IParameters::MuxerSavingFileStrategy strategy, IParameters::MuxerSavingCondition condition = IParameters::MuxerSavingCondition_InputPTS, IParameters::MuxerAutoFileNaming naming = IParameters::MuxerAutoFileNaming_ByNumber, AM_UINT param = DefaultAutoSavingParam, AM_UINT channel = DInvalidGeneralIntParam) = 0;
    virtual AM_ERR SetMaxFileNumber(AM_UINT max_file_num, AM_UINT channel = DInvalidGeneralIntParam) = 0;
    virtual AM_ERR SetTotalFileNumber(AM_UINT total_file_num, AM_UINT channel = DInvalidGeneralIntParam) = 0;
    virtual AM_ERR NewStream(AM_UINT out_index, AM_UINT& stream_index, IParameters::StreamType type, IParameters::StreamFormat format) = 0;
    virtual AM_ERR StartRecord() = 0;
    virtual AM_ERR StopRecord() = 0;

    virtual void AbortRecord() = 0;

    virtual AM_ERR PauseRecord() = 0;
    virtual AM_ERR ResumeRecord() = 0;

    virtual AM_ERR CloseAudioHAL() = 0;
    virtual AM_ERR ReopenAudioHAL() = 0;

    //virtual void SetGetPridataAppCallback();

    //total set params, todo
    //virtual AM_ERR GetCurrentParams() = 0;
    //virtual AM_ERR SetAllParams() = 0;

     virtual AM_ERR DisableVideo() = 0;
     virtual AM_ERR DisableAudio() = 0;

    //video parameters (optional) api:
    virtual AM_ERR SetVideoStreamDimention(AM_UINT out_index, AM_UINT stream_index, AM_UINT width, AM_UINT height) = 0;
    virtual AM_ERR SetVideoStreamFramerate(AM_UINT out_index, AM_UINT stream_index, AM_UINT framerate_num, AM_UINT framerate_den) = 0;
    virtual AM_ERR SetVideoStreamBitrate(AM_UINT out_index, AM_UINT stream_index, AM_UINT bitrate) = 0;
    virtual AM_ERR SetVideoStreamLowdelay(AM_UINT out_index, AM_UINT stream_index, AM_UINT lowdelay) = 0;
    virtual AM_ERR SetVideoStreamEntropyType(AM_UINT out_index, AM_UINT stream_index, IParameters::EntropyType entropy_type) = 0;
    virtual AM_ERR DemandIDR(AM_UINT out_index) = 0;
    virtual AM_ERR UpdateGOPStructure(AM_UINT out_index, AM_INT M, AM_INT N, AM_INT idr_interval) = 0;
    virtual AM_ERR UpdatePreviewDisplay(AM_UINT width, AM_UINT height, AM_UINT pos_x, AM_UINT pos_y, AM_UINT alpha) = 0;

    virtual AM_ERR SetDisplayMode(AM_UINT display_preview, AM_UINT display_playback, AM_UINT preview_vout, AM_UINT pb_vout) = 0;
    virtual AM_ERR GetDisplayMode(AM_UINT& display_preview, AM_UINT& display_playback, AM_UINT& preview_vout, AM_UINT& pb_vout) = 0;

    //audio parameters (optional) api:
    virtual AM_ERR SetAudioStreamChannelNumber(AM_UINT out_index, AM_UINT stream_index, AM_UINT channel) = 0;
    virtual AM_ERR SetAudioStreamSampleFormat(AM_UINT out_index, AM_UINT stream_index, IParameters::AudioSampleFMT sample_fmt) = 0;
    virtual AM_ERR SetAudioStreamSampleRate(AM_UINT out_index, AM_UINT stream_index, AM_UINT samplerate) = 0;
    virtual AM_ERR SetAudioStreamBitrate(AM_UINT out_index, AM_UINT stream_index, AM_UINT bitrate) = 0;

    //subtitle parameters (optional) api: todo

    //pravate data api:
    virtual AM_ERR AddPrivateDataPacket(AM_U8* data_ptr, AM_UINT len, AM_U16 data_type, AM_U16 sub_type) = 0;
    virtual AM_ERR SetPrivateDataDuration(AM_UINT duration, AM_U16 data_type) = 0;

    //max-duration, max-filesize
    virtual AM_ERR SetMaxFileDuration(AM_UINT out_index,AM_U64 timeUs) = 0;
    virtual AM_ERR SetMaxFileSize(AM_UINT out_index,AM_U64 bytes) = 0;

    //debug api
    virtual AM_ERR ExitPreview() = 0;//should be called after stop encoding, rectest would call this, app would not, camara will handle the dsp state
    virtual AM_ERR SetProperty(AM_UINT prop, AM_UINT value) = 0;

    virtual AM_INT AddOsdBlendArea(AM_INT xPos, AM_INT yPos, AM_INT width, AM_INT height) = 0;
    virtual AM_ERR UpdateOsdBlendArea(AM_INT index, AM_U8* data, AM_INT width, AM_INT height) = 0;
    virtual AM_ERR RemoveOsdBlendArea(AM_INT index) = 0;

    //CLUT mode
    virtual AM_INT AddOsdBlendAreaCLUT(AM_INT xPos, AM_INT yPos, AM_INT width, AM_INT height) = 0;
    virtual AM_ERR UpdateOsdBlendAreaCLUT(AM_INT index, AM_U8* data, AM_U32* p_input_clut, AM_INT width, AM_INT height, IParameters::OSDInputDataFormat data_format) = 0;
    virtual AM_ERR RemoveOsdBlendAreaCLUT(AM_INT index) = 0;

    virtual AM_ERR FreezeResumeHDMIPreview(AM_INT flag) = 0;

    //raw data capture
    virtual AM_ERR CaptureYUV(char* name, SYUVData* yuv = NULL) = 0;

    //capture jpeg during encoding
    virtual AM_ERR CaptureJPEG(const char* name) = 0;

    virtual void PrintState() = 0;
};

extern IRecordControl* CreateRecordControl();
extern IRecordControl2* CreateRecordControl2(int streaming_only = 0);
extern IRecordControl2* CreateRecordControlDuplex();
extern IRecordControl* CreateAuthorControl();

//----------------------------------------------------------------------------------

extern const AM_IID IID_IAmbaAuthorControl;
//-----------------------------------------------------------------------
//
// IAmbaAuthorControl
//
//-----------------------------------------------------------------------
class IAmbaAuthorControl: public IMediaControl
{
public:
	typedef enum {
		STATE_STOPPED,
		STATE_RECORDING,
		STATE_STOPPING,
	} STATE;

	struct INFO {
		STATE	state;
	};
public:
	DECLARE_INTERFACE(IAmbaAuthorControl, IID_IAmbaAuthorControl);
	virtual AM_ERR StartRecord() = 0;
	virtual AM_ERR StopRecord() = 0;
	virtual void AbortRecord() = 0;
	virtual AM_ERR SetOutputFile(const char *pFileName) = 0;
	virtual void GetAudioParam(AM_INT * samplingRate, AM_INT * numOfChannels, AM_INT * bitrate, AM_INT * sampleFmt, AM_INT * codecId) = 0;
	virtual void SetAudioParam(AM_INT samplingRate, AM_INT numOfChannels, AM_INT bitrate, AM_INT sampleFmt, AM_INT codecId) = 0;
	virtual void GetVideoParam(AM_INT * width, AM_INT * height, AM_INT * framerate, AM_INT * quality) = 0;
	virtual void SetVideoParam(AM_INT width, AM_INT height, AM_INT framerate, AM_INT quality) = 0;
};
extern IAmbaAuthorControl* CreateAmbaAuthorControl();
#endif

