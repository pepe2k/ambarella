/*
 * g_muxer_ffmpeg.h
 *
 * History:
 *    2012/6/15 - [QingXiong Z] create file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __G_MUXER_FFMPEG_H__
#define __G_MUXER_FFMPEG_H__

extern "C" {
//#define INT64_C
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}
#if (PLATFORM_LINUX && TARGET_USE_AMBARELLA_I1_DSP)
#include "general_loader_filter.h"
#endif

#define MUXER_MAX_BUFFER_VIDEO_DATA 120
#define MUXER_MAX_BUFFER_AUDIO_DATA 120
#define AMBA_AAC 1


class IFileWriter;
class AVFormatContext;
class AVOutputFormat;
class CGMuxerFFMpeg : public IGMuxer, public CActiveObject
{
#define TRANSCODER_INDEX 0xff

    typedef CActiveObject inherited;
    enum{
        CMD_FULL = CMD_GMF_LAST + 100,
        CMD_GOON,
        CMD_FINISH,
        CMD_MD_EVENT,
    };
    enum{
        //cut file with according to master input, not very precise pts, but less latency for write file/streaming
        STATE_SAVING_PARTIAL_FILE = LAST_COMMON_STATE + 1,
        STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_MASTER_PIN,
        STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN,
        STATE_SAVING_PARTIAL_FILE_WAIT_ONE_PIN,//master or non master
        STATE_SAVING_PRESET_TOTAL_FILE,
    };
    typedef struct STREAM_INFO{
        //AM_UINT Type;
        AVStream* pStream;
        bool bStreamStart;
        AM_U64 FirstPTS;//the whole stream
        AM_U64 LastPTS;//the whole stream
        AM_U64 StartPTS;//every separated file
        AM_U64 EndPTS;//every separated file
        AM_U64 CurrentFrameCount;//every separated file's frame count
        AM_U64 NextFileTimeThreshold;//audio is different from video
        bool bNextFileTimeThresholdSet;
        bool bAutoBoundaryReached;
        bool bAutoBoundaryStarted;//a new separated file start

        bool bCachedBuffer;//have cached buffer?
        CGBuffer CachedBuffer;//saving for Next File
    }STREAM_INFO;
public:
    static IGMuxer* Create(CGeneralMuxer* manager, AM_UINT index);

public:
    void* GetInterface(AM_REFIID refiid)
    {
        return inherited::GetInterface(refiid);
    }
    virtual void Delete();

    virtual AM_ERR ConfigMe(CUintMuxerConfig* con);
    virtual AM_ERR UpdateConfig(CUintMuxerConfig* con);
    virtual AM_ERR PerformCmd(CMD& cmd, AM_BOOL isSend);
    virtual AM_ERR FeedData(CGBuffer* buffer);
    virtual AM_ERR FinishMuxer();
    virtual AM_ERR QueryInfo(AM_INT type, CParam& par);
    virtual AM_ERR Dump();
    virtual AM_ERR postMDMsg(AM_INT msg);

    virtual void OnRun();
private:
    CGMuxerFFMpeg(CGeneralMuxer* manager, AM_UINT index);
    AM_ERR Construct();
    ~CGMuxerFFMpeg();
    AM_ERR ClearQueue(CQueue* queue);
private:
    AM_ERR SetupMuxerEnv();
    AM_ERR SetupVideoEnv();
    AM_ERR SetupAudioEnv();
    AM_ERR FinishMuxerEnv();

    AM_ERR DoStop();
    AM_ERR DoFinish();
    AM_ERR DoFull(CMD& cmd);
    AM_ERR DogetMD(CMD& cmd);
    AM_ERR ProcessCmd(CMD& cmd);
    AM_ERR ProcessData(CGBuffer* pBuffer);
    AM_ERR ProcessVideoData(CGBuffer* pBuffer);
    AM_ERR ProcessAudioData(CGBuffer* pBuffer);
    AM_UINT ReadBit(AM_U8 *pBuffer, AM_INT *value, AM_U8 *bit_pos = 0, AM_UINT num = 1);
private:
    class SaveInfo
    {
        public:
        SaveInfo():
            saveSize(0),
            saveFrame(0)
        {}
        AM_INT saveSize;
        AM_INT saveFrame;
    };
private:
    CGeneralMuxer* mpManager;
    AM_INT mIndex;
    CUintMuxerConfig* mpConfig;

    IFileWriter* mpWriter;
    CQueue* mpVideoQ;
    CQueue* mpAudioQ;

    CGBuffer mBuffer;
    CGBuffer mBufferDump;
    volatile AM_BOOL mbFlowFull;

    SaveInfo mInfo;
    AM_INT mbMDFileCreated;

    //save env
    AVFormatContext* mpFormat;
    AVOutputFormat* mpOutFormat;
    AM_U8 mAudioExtra[2];

    AM_BOOL mNeedParseSPS;//for gmf this is not need
    AM_U64 mVideoPts;
    AM_INT mIDR;

//auto separate file
private:
    AM_ERR Initialize();
    AM_ERR Finalize();
    AM_ERR InitMD();
    AM_ERR FinalizeMD();
    void ClearFFMpegContext();
    bool IsCommingBufferAutoFileBoundary(CGBuffer* pBuffer);
    void updateFileName(AM_UINT file_index);

public:
    //This is one api of class CGMuxerFFMpeg
    virtual AM_ERR SetSavingTimeDuration( AM_UINT duration, AM_UINT maxfilecount);

    virtual AM_ERR EnableLoader(AM_BOOL flag);
    virtual AM_ERR ConfigLoader(char* path, char* m3u8name, char* host, int count);

private:
    AM_UINT mMuxerIndex;
    STREAM_INFO mVideoStreamInfo;
    STREAM_INFO mAudioStreamInfo;

    char* mpOutputFileName;
    char* mpBaseFileName;
    char* mpFileFormate;//.ts .mp4 .es ...

    AM_UINT mMasterStreamType;
    IParameters::MuxerSavingFileStrategy mSavingFileStrategy;
    IParameters::MuxerSavingCondition mSavingCondition;
    IParameters::MuxerAutoFileNaming mAutoFileNaming;

    AM_UINT mAutoSaveFrameCount;
    AM_U64 mAutoSavingTimeDuration;
    AM_UINT mIDRFrameInterval;//mIDRFrameCountInterval * mFrameInterval

    //to caculate IDR frame interval
    AM_UINT mFrameInterval;
    AM_UINT mIDRFrameCountInterval;//default value is 60 frames
    bool mbSavingTimeDurationChanged;
    AM_UINT mDuration;
     AM_UINT mMaxFileCount;
   bool mbIDRFrameCountIntervalConfirmed;//false: need to check mIDRFrameCountInterval is correct or not.
    AM_U64 mIDRFramePTSOne;//to caculate mIDRFrameCountInterval
    bool mbCheckAudioFirstPts;
    AM_U64 mLastFramePTS;

private:
    AM_U64 mFileDuration;
    AM_UINT mFileBitrate;
    AM_UINT mOutputFileNameLength;
    AM_U64 mCurrentTotalFilesize;
    AM_UINT mCurrentFileIndex;

    bool mbTranscodeStream;

private:
    AM_BOOL mbLoaderEnabled;
#if (PLATFORM_LINUX && TARGET_USE_AMBARELLA_I1_DSP)
    CGeneralLoader* mpLoader;
#endif
};
#endif
