
/*
 * ffmpeg_demuxer.h
 *
 * History:
 *    2010/1/27 - [Oliver Li] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __FFMPEG_DEMUXER_H__
#define __FFMPEG_DEMUXER_H__

class CFFMpegDemuxer;
class CFFMpegDemuxerOutput;

#if !FFMPEG_VER_0_6
#define CODEC_TYPE_NB AVMEDIA_TYPE_NB
#endif

//-----------------------------------------------------------------------
//
// CFFMpegBufferPool
//
//-----------------------------------------------------------------------
class CFFMpegBufferPool: public CSimpleBufferPool
{
	typedef CSimpleBufferPool inherited;

public:
	static CFFMpegBufferPool* Create(const char *name, AM_UINT count);

protected:
	CFFMpegBufferPool(const char *name):
		inherited(name)
	{}
	AM_ERR Construct(AM_UINT count)
	{
		return inherited::Construct(count, sizeof(CBuffer) + sizeof(AVPacket));
	}
	virtual ~CFFMpegBufferPool() {}

protected:
	virtual void OnReleaseBuffer(CBuffer *pBuffer);
};

//-----------------------------------------------------------------------
//
// CFFMpegDemuxer
//
//-----------------------------------------------------------------------
class CFFMpegDemuxer: public CActiveFilter, public IDemuxer, public IDataRetriever
{
	typedef CActiveFilter inherited;
	friend class CFFMpegDemuxerOutput;
         enum{
            CMD_A5S_PLAYMODE = CMD_LAST + 100,
            CMD_A5S_PLAYNM,
         };

    typedef struct {
        AM_U8* pdata;
        AM_U32 data_len;
        AM_U32 total_length;
        AM_U16 data_type;
        AM_U16 sub_type;
    } SPriDataPacket;

public:
	static int ParseMedia(struct parse_file_s *pParseFile, struct parser_obj_s *pParser);

public:
	static IFilter* Create(IEngine *pEngine);

protected:
	CFFMpegDemuxer(IEngine *pEngine):
        inherited(pEngine, "FFMpegDemuxer"),
        mpFormat(NULL),
        mVideo(-1),
        mAudio(-1),
        mSubtitle(-1),
        mPridata(-1),
        mWidth(0),
        mHeight(0),
        mpVideoOutput(NULL),
        mpAudioOutput(NULL),
        mpSubtitleOutput(NULL),
        mpPridataOutput(NULL),
        mpVideoBP(NULL),
        mpAudioBP(NULL),
        mpSubtitleBP(NULL),
        mpPridataBP(NULL),
        mpBP(NULL),
        mpOutput(NULL),
        mLastPts(AV_NOPTS_VALUE),
        mAudPtsSave4LeapChk(AV_NOPTS_VALUE),
        mbEnableVideo(true),
        mbEnableVideoBakFlag(false),
        mbEnableAudio(true),
        mbEnableSubtitle(true),
        mbEnablePridata(true),
        mbVideoDataComes(false),
        mbAudioDataComes(false),
        mbFilterBlockedMsgSent(false),
        mbFlushFlag(false),
        mbEOSAlreadySent(false),
        mPTSVideo_Num(1),
        mPTSVideo_Den(1),
        mPTSAudio_Num(1),
        mPTSAudio_Den(1),
        mSeekByByte(-1),
        mDurationEstimated(-1),
        mDurationEst(0),
        mIndex(-1),
        mEstimatedPTS(0),
        mbNVRRTSP(false)
        {
#ifdef AM_DEBUG
            DumpEsInit();
#endif
        }
	AM_ERR Construct();
	virtual ~CFFMpegDemuxer();

protected:
	void Clear();
	virtual bool ProcessCmd(CMD& cmd);
	void DoPause();
	void DoResume();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete();
	virtual void SetIndex(AM_INT index){mIndex = index;};

    // IFilter
    virtual void GetInfo(INFO& info);
    virtual IPin* GetOutputPin(AM_UINT index);
#ifdef AM_DEBUG
    virtual void PrintState();
#endif

    // called when stopped, and received CMD_RUN
    virtual void OnRun();

    // IDemuxer
    virtual AM_ERR LoadFile(const char *pFileName, void *pParserObj);
    virtual bool isSupportedByDuplex();
    virtual AM_ERR Seek(AM_U64 &ms);
    virtual AM_ERR GetTotalLength(AM_U64& ms);
    virtual AM_ERR GetFileType(const char *&pFileType);
    virtual void GetVideoSize(AM_INT * pWidth, AM_INT * pHeight);

    virtual void EnableAudio(bool enable);
    virtual void EnableVideo(bool enable);
    virtual void EnableSubtitle(bool enable);

    //IDataRetriever
    virtual AM_ERR RetieveData(AM_U8* pdata, AM_UINT max_len, IParameters::DataCategory data_category = IParameters::DataCategory_PrivateData);
    virtual AM_ERR RetieveDataByType(AM_U8* pdata, AM_UINT max_len, AM_U16 type, AM_U16 sub_type, IParameters::DataCategory data_category = IParameters::DataCategory_PrivateData);
    virtual AM_ERR SetDataRetrieverCallBack(void* cookie, data_retriever_callback_t callback) {AM_ASSERT(0); return ME_OK;}//not needed here


private:
    void ConvertVideoPts(am_pts_t& pts);
    void ConvertAudioPts(am_pts_t& pts);

    void UpdatePTSConvertor();

    AM_UINT CheckDecType(SCODECID decID);
    AM_ERR EstimateDuration();

    /*A5S DV Playback Test */
    virtual AM_ERR  A5SPlayMode(AM_UINT mode);
    virtual AM_ERR  A5SPlayNM(AM_INT start_n, AM_INT end_m);
    AM_ERR  DoA5SPlayMode(AM_UINT mode);
    AM_ERR  DoA5SPlayNM(AM_INT start_n, AM_INT end_m);
    /*end A5S DV Test*/

private:
    void postPrivateDataMsg(AM_U32 size, AM_U16 type, AM_U16 sub_type);
    void storePrivateData(AM_U8* pdata, AM_UINT max_len);
    void freeAllPrivateData();
    void printBytes(AM_U8* p, AM_UINT size);

private:
    AVFormatContext *mpFormat;
    AM_INT mVideo;
    AM_INT mAudio;
    AM_INT mSubtitle;
    AM_INT mPridata;
    AM_INT mWidth;
    AM_INT mHeight;

    CFFMpegDemuxerOutput *mpVideoOutput;
    CFFMpegDemuxerOutput *mpAudioOutput;
    CFFMpegDemuxerOutput *mpSubtitleOutput;
    CFFMpegDemuxerOutput *mpPridataOutput;

    CFFMpegBufferPool *mpVideoBP;
    CFFMpegBufferPool *mpAudioBP;
    CFFMpegBufferPool *mpSubtitleBP;
    CFFMpegBufferPool *mpPridataBP;

    CFFMpegBufferPool* mpBP;
    CFFMpegDemuxerOutput* mpOutput;

    AM_INT mWantedStream[CODEC_TYPE_NB];
    AVPacket mPkt;
    AM_U64 mLastPts;
    AM_U64 mAudPtsSave4LeapChk;
    bool mbEnableVideo,mbEnableVideoBak,mbEnableVideoBakFlag;
    bool mbEnableAudio;
    bool mbEnableSubtitle;
    bool mbEnablePridata;

    //checking the first audio/video data comes yet
    bool mbVideoDataComes;
    bool mbAudioDataComes;
    bool mbFilterBlockedMsgSent;

    // usage: fill video pts after seek if pts is negative.
    //        solves issues -- no video pts after seek in different formats
    // bug list:  917/921/
    bool mbFlushFlag;

    //for bug#2450
    bool mbEOSAlreadySent;

private:
    int FindMediaStream(AVFormatContext *pFormat, int media);
    bool SendEOS(CFFMpegDemuxerOutput *pPin);
    int hasCodecParameters(AVCodecContext *enc);

private:
    AM_UINT mPTSVideo_Num;
    AM_UINT mPTSVideo_Den;
    AM_UINT mPTSAudio_Num;
    AM_UINT mPTSAudio_Den;

    AM_INT mSeekByByte;
    AM_INT mDurationEstimated;
    AM_U64 mDurationEst;
    AM_INT mIndex;

private:
    am_pts_t mEstimatedPTS;//private data has no pts
    CDoubleLinkedList mPridataList;

#ifdef AM_DEBUG
    AM_INT log_video_frame_index;
    AM_INT log_audio_frame_index;
    AM_INT log_subtitle_index;
    AM_INT log_start_frame;
    AM_INT log_end_frame;
    void DumpEsInit(){
        log_video_frame_index = 0;
        log_audio_frame_index = 0;
        log_subtitle_index = 0;
        get_dump_config(&log_start_frame, &log_end_frame);
    }
    void DumpEs(unsigned char *buf, int len,int stream_index);
    void DumpStreamEs(unsigned char *buf, int len,const char *name,int index);
#endif

private:
    bool mbNVRRTSP;
};

//-----------------------------------------------------------------------
//
// CFFMpegDemuxerOutput
//
//-----------------------------------------------------------------------
class CFFMpegDemuxerOutput: public COutputPin
{
	typedef COutputPin inherited;
	friend class CFFMpegDemuxer;

public:
	static CFFMpegDemuxerOutput* Create(CFilter *pFilter);

protected:
	CFFMpegDemuxerOutput(CFilter *pFilter);
	AM_ERR Construct();
	virtual ~CFFMpegDemuxerOutput();

public:
	AM_ERR InitStream(AVFormatContext *pFormat, int stream, int media,int divx_packed = 0);

public:
	virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat)
	{
		AM_DBG("***GetMediaFormat %p.\n", &mMediaFormat);
		pMediaFormat = &mMediaFormat;
		return ME_OK;
	}

private:
	CMediaFormat mMediaFormat;
	AM_INT mStream;
	SConsistentConfig* mpSharedRes;
};

#endif

