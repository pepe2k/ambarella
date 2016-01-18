
/**
 * record_if.h
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

#ifndef __RECORD_IF_H__
#define __RECORD_IF_H__

extern const AM_IID IID_IRecordEngine;
extern const AM_IID IID_IMuxer;
extern const AM_IID IID_IMuxerControl;
extern const AM_IID IID_IEncoder;
extern const AM_IID IID_IVideoControl;
extern const AM_IID IID_IVideoEncoder;
extern const AM_IID IID_IAudioInput;
extern const AM_IID IID_IAudioEncoder;
extern const AM_IID IID_IPridataComposer;
extern const AM_IID IID_IFileWriter;


//-----------------------------------------------------------------------
//
// IRecordEngine
//
//-----------------------------------------------------------------------
class IRecordEngine: public IEngine
{
	typedef IEngine inherited;

public:
    enum {
        MSG_RECORD_STOPPED = inherited::MSG_LAST,
        MSG_RECORD_PAUSE_DONE,
        MSG_RECORD_RESUME_DONE,
        MSG_RECORD_FINALIZE_DONE
    };

public:
	DECLARE_INTERFACE(IRecordEngine, IID_IRecordEngine);
};


//-----------------------------------------------------------------------
//
// IMuxer
//
//-----------------------------------------------------------------------
class IMuxer: public IInterface
{
public:
	DECLARE_INTERFACE(IMuxer, IID_IMuxer);
	virtual AM_ERR SetOutputFile(const char *pFileName) = 0;
	virtual AM_ERR SetThumbNailFile(const char *pThumbNailFileName) = 0;
};

//-----------------------------------------------------------------------
//
// IMuxerControl
//
//-----------------------------------------------------------------------
class IMuxerControl: public IParameters
{
public:
    DECLARE_INTERFACE(IMuxerControl, IID_IMuxerControl);
    virtual AM_ERR SetContainerType(IParameters::ContainerType container_type) = 0;
    virtual AM_ERR SetSavingStrategy(IParameters::MuxerSavingFileStrategy strategy, IParameters::MuxerSavingCondition condition, IParameters::MuxerAutoFileNaming naming, AM_UINT param = DefaultAutoSavingParam, bool PTSstartFromZero = true) = 0;
    virtual AM_ERR SetMaxFileNumber(AM_UINT max_file_num) = 0;
    virtual AM_ERR SetTotalFileNumber(AM_UINT total_file_num) = 0;
    virtual AM_ERR SetPresetMaxFrameCount(AM_U64 max_frame_count, IParameters::StreamType type = IParameters::StreamType_Video) = 0;
    virtual AM_ERR SetPresetMaxFilesize(AM_U64 max_file_size) = 0;
};

//-----------------------------------------------------------------------
//
// IEncoder     (deprecated)
//
//-----------------------------------------------------------------------
class IEncoder: public IInterface
{
public:
	typedef enum {
		SA_NONE,
		SA_STOP,
		SA_ABORT,
	} STOP_ACTION;

public:
	DECLARE_INTERFACE(IEncoder, IID_IEncoder);
	virtual AM_ERR StopEncoding() = 0;
};

//-----------------------------------------------------------------------
//
// IVideoEncoder
//
//-----------------------------------------------------------------------
class IVideoEncoder: public IParameters
{
public:
    typedef enum {
        SA_NONE,
        SA_STOP,
        SA_ABORT,
    } STOP_ACTION;

public:
    DECLARE_INTERFACE(IVideoEncoder, IID_IVideoEncoder);
    virtual AM_ERR StopEncoding() = 0;
    virtual AM_ERR SetModeConfig(SEncodingModeConfig* pconfig) = 0;
    virtual AM_ERR ExitPreview() = 0;//rectest only, should only be called after StopEncoding
    virtual AM_ERR UpdatePreviewDisplay(AM_UINT width, AM_UINT height, AM_UINT pos_x, AM_UINT pos_y, AM_UINT alpha) = 0;
    virtual AM_ERR DemandIDR(AM_UINT out_index) = 0;
    virtual AM_ERR UpdateGOPStructure(AM_UINT out_index, AM_INT M, AM_INT N, AM_INT idr_interval) = 0;

    virtual AM_ERR CaptureRawData(AM_U8*& p_raw, AM_UINT target_width, AM_UINT target_height, IParameters::PixFormat pix_format) = 0;
    virtual AM_ERR CaptureJpeg(char* jpeg_filename, AM_UINT target_width, AM_UINT target_height) = 0;
    virtual AM_ERR CaptureYUVdata(AM_INT fd, AM_UINT& pitch, AM_UINT& height, SYUVData* yuvdata = NULL) = 0;

    virtual AM_ERR SetDisplayMode(AM_UINT display_preview, AM_UINT display_playback, AM_UINT preview_vout, AM_UINT pb_vout) = 0;
    virtual AM_ERR GetDisplayMode(AM_UINT& display_preview, AM_UINT& display_playback, AM_UINT& preview_vout, AM_UINT& pb_vout) = 0;

    virtual AM_INT AddOsdBlendArea(AM_INT xPos, AM_INT yPos, AM_INT width, AM_INT height) = 0;
    virtual AM_ERR UpdateOsdBlendArea(AM_INT index, AM_U8* data, AM_INT width, AM_INT height) = 0;
    virtual AM_ERR RemoveOsdBlendArea(AM_INT index) = 0;

    //CLUT mode
    virtual AM_INT AddOsdBlendAreaCLUT(AM_INT xPos, AM_INT yPos, AM_INT width, AM_INT height) = 0;
    virtual AM_ERR UpdateOsdBlendAreaCLUT(AM_INT index, AM_U8* data, AM_U32* p_input_clut, AM_INT width, AM_INT height, IParameters::OSDInputDataFormat data_format) = 0;
    virtual AM_ERR RemoveOsdBlendAreaCLUT(AM_INT index) = 0;

    virtual AM_ERR FreezeResumeHDMIPreview(AM_INT flag) = 0;
};

//-----------------------------------------------------------------------
//
// IVideoControl
//
//-----------------------------------------------------------------------

class IVideoControl: public IInterface
{
public:
    DECLARE_INTERFACE(IVideoControl, IID_IVideoControl);
    virtual AM_ERR SetVideoParameters(AM_UINT pic_width, AM_UINT pic_height,
            AM_UINT framerate_num, AM_UINT framerate_den, AM_UINT sample_aspect_ratio_num,
            AM_UINT sample_aspect_ratio_den, AM_UINT lowdelay) = 0;
};

//-----------------------------------------------------------------------
//
// IAudioInput
//
//-----------------------------------------------------------------------
class IAudioInput: public IParameters
{
public:
    DECLARE_INTERFACE(IAudioInput, IID_IAudioInput);
};

//-----------------------------------------------------------------------
//
// IAudioEncoder
//
//-----------------------------------------------------------------------
class IAudioEncoder: public IParameters
{
public:
    DECLARE_INTERFACE(IAudioEncoder, IID_IAudioEncoder);
};

//-----------------------------------------------------------------------
//
// IPridataComposer
//
//-----------------------------------------------------------------------
class IPridataComposer: public IParameters
{
public:
    DECLARE_INTERFACE(IPridataComposer, IID_IPridataComposer);
    virtual AM_ERR SetGetPridataCallback(AM_CallbackGetUnPackedPriData f_unpacked, AM_CallbackGetPackedPriData f_packed) = 0;
    virtual AM_ERR SetPridata(AM_U8* rawdata, AM_UINT len, AM_U16 data_type, AM_U16 sub_type) = 0;
    virtual AM_ERR SetPridataDuration(AM_UINT duration, AM_U16 data_type) = 0;
};

//-----------------------------------------------------------------------
//
// IFileWriter
//
//-----------------------------------------------------------------------
class IFileWriter: public IInterface
{
public:
	DECLARE_INTERFACE(IFileWriter, IID_IFileWriter);
	virtual AM_ERR CreateFile(const char *pFileName, bool isApend=false) = 0;
	virtual void CloseFile() = 0;
	virtual AM_ERR SeekFile(am_file_off_t offset) = 0;
	virtual AM_ERR WriteFile(const void *pBuffer, AM_UINT size) = 0;
};


//-----------------------------------------------------------------------
//
// CFileWriter
//
//-----------------------------------------------------------------------
class CFileWriter: public CObject, public IFileWriter
{
	typedef CObject inherited;

public:
	static CFileWriter* Create();

public:
	CFileWriter(): mpFile(NULL) {}
	AM_ERR Construct();
	virtual ~CFileWriter();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid)
	{
		if (refiid == IID_IFileWriter)
			return (IFileWriter*)this;
		return inherited::GetInterface(refiid);
	}
	virtual void Delete() { return inherited::Delete(); }

	// IFileWriter
	virtual AM_ERR CreateFile(const char *pFileName, bool isApend);
	virtual void CloseFile();
	virtual AM_ERR SeekFile(am_file_off_t offset);
	virtual AM_ERR WriteFile(const void *pBuffer, AM_UINT size);

private:
	void __CloseFile();

private:
	void *mpFile;
};

extern IFilter* CreateFFMpegDemuxer(IEngine *pEngine);
extern IFilter* CreateAmbaVideoSink(IEngine *pEngine);
extern IFilter* CreateFFMpegDecoder(IEngine *pEngine);
extern IFilter* CreateAudioEffecter(IEngine *pEngine);
extern IFilter* CreateAudioRenderer(IEngine *pEngine);
extern IFilter* CreatePridataParser(IEngine * pEngine);

extern IFilter* CreateAmbaVideoEncoderFilter(IEngine *pEngine, AM_INT fd);
extern IFilter* CreateITronEncoderFilter(IEngine *pEngine, bool bDual);
extern IFilter* CreateVideoEncoderFilter(IEngine *pEngine, AM_UINT request_stream_number);
extern IFilter* CreateSimpleMuxer(IEngine *pEngine);
extern IFilter* CreateFFmpegMuxer(IEngine *pEngine);
extern IFilter* CreateFFmpegMuxer2(IEngine *pEngine);
extern IFilter* CreateAudioInput(IEngine *pEngine);
extern IFilter* CreateAudioInput2(IEngine *pEngine);
extern IFilter* CreateAACEncoder(IEngine *pEngine);
extern IFilter* CreateAudioEncoder(IEngine *pEngine);
extern IFilter* CreateAudioMuxer(IEngine *pEngine);
extern IFilter* CreateFFmpegMuxerEx(IEngine *pEngine);
extern IFilter* CreatePridataComposer(IEngine * pEngine);

extern IFilter* CreateAmbaVideoEncoderFilterEx(IEngine *pEngine);
extern IFilter* CreateAmbaAudioInput(IEngine *pEngine);
extern IFilter* CreateAmbaAudioEncoder(IEngine *pEngine);
extern IFilter* CreateFFmpegAudioEncoder(IEngine *pEngine);
extern IFilter* CreateAmbaFFMpegMuxer(IEngine *pEngine);
extern IFilter* CreateAmbaMp4Muxer(IEngine *pEngine);

extern IFilter* CreateRTSPServer(IEngine *pEngine);


#endif

