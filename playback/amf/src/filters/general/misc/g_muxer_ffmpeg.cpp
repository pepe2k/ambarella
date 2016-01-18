/*
 * g_muxer_ffmpeg.cpp
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

#include "general_header.h"
#include "general_interface.h"
#include "general_parse.h"

#include "record_if.h"

#include "general_muxer_save.h"
#include "g_muxer_ffmpeg.h"

//config
#define cutfile_with_precise_pts 1
//#define DEBUG_NVR_OVERNIGHT
#define MAX_PTS_DIFF  (90000*5)
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
IGMuxer* CGMuxerFFMpeg::Create(CGeneralMuxer* manager, AM_UINT index)
{
    CGMuxerFFMpeg* result = new CGMuxerFFMpeg(manager, index);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
        }
    return result;
}

CGMuxerFFMpeg::CGMuxerFFMpeg(CGeneralMuxer* manager, AM_UINT index):
    inherited("CGMuxerFFMpeg"),
    mpManager(manager),
    mpConfig(NULL),
    mpVideoQ(NULL),
    mpAudioQ(NULL),
    mbFlowFull(AM_FALSE),
    mMuxerIndex(index),
    mpOutputFileName(NULL),
    mpBaseFileName(NULL),
    mpFileFormate(NULL),
    mFrameInterval(0),
    mIDRFrameCountInterval(60),
    mbSavingTimeDurationChanged(false),
    mDuration(0),
    mMaxFileCount(0),
    mbIDRFrameCountIntervalConfirmed(true),
    mIDRFramePTSOne(0),
    mbCheckAudioFirstPts(true),
    mLastFramePTS(0),
    mFileDuration(0),
    mFileBitrate(0),
    mCurrentTotalFilesize(0),
    mCurrentFileIndex(0),
    mbLoaderEnabled(AM_FALSE)
{
    mpFormat = NULL;
    mpOutFormat = NULL;
    mNeedParseSPS = AM_FALSE;
    mVideoPts = 0;
    mIDR = 0;
    mbMDFileCreated = 0;
#if (PLATFORM_LINUX && TARGET_USE_AMBARELLA_I1_DSP)
    mpLoader = NULL;
#endif

    mMasterStreamType = STREAM_VIDEO;
    mVideoStreamInfo.pStream = NULL;
    mAudioStreamInfo.pStream = NULL;
    mVideoStreamInfo.bStreamStart = false;
    mAudioStreamInfo.bStreamStart = false;
    mVideoStreamInfo.bCachedBuffer = false;
    mAudioStreamInfo.bCachedBuffer = false;//now audio stream doesn't use it
    mSavingFileStrategy = IParameters::MuxerSavingFileStrategy_ToTalFile;
    mSavingCondition = IParameters::MuxerSavingCondition_Invalid;

    if(index==TRANSCODER_INDEX) mbTranscodeStream = true;
    else mbTranscodeStream = false;
}

AM_ERR CGMuxerFFMpeg::Construct()
{
    AM_INFO("CGMuxerFFMpeg Construct!\n");
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    CQueue* mainQ = MsgQ();
    if ((mpVideoQ= CQueue::Create(mainQ, this, sizeof(CGBuffer), MUXER_MAX_BUFFER_VIDEO_DATA)) == NULL)
        return ME_NO_MEMORY;

    if ((mpAudioQ= CQueue::Create(mainQ, this, sizeof(CGBuffer), MUXER_MAX_BUFFER_AUDIO_DATA)) == NULL)
        return ME_NO_MEMORY;

    if ((mpWriter = CFileWriter::Create()) == NULL) return ME_ERROR;

    //SendCmd(CMD_RUN);
    //mpWorkQ->SetThreadPrio(1, 1);

    mOutputFileNameLength = 128;
    mpOutputFileName = (char*)malloc(mOutputFileNameLength);
    mpFileFormate = (char*)malloc(10);
    return ME_OK;
}

AM_ERR CGMuxerFFMpeg::SetupMuxerEnv()
{
    AM_INFO("SetupMuxerEnv for FFMpeg ...\n");
    AM_ASSERT(mpConfig != NULL);
    AM_ASSERT(mpOutputFileName != NULL);

    AM_ERR err;
    AVFormatParameters params, *ap = &params;
//    enum CodecID codec_id;

    if(mpConfig->configVideo != AM_TRUE && mpConfig->configAudio != AM_TRUE){
        AM_ASSERT(0);
        return ME_ERROR;
    }

    //guess format
    mpOutFormat = av_guess_format(NULL, mpOutputFileName, NULL);
    if (!mpOutFormat) {
    	AM_ERROR("Could not deduce output format from file extension: using MPEG.\n");
    	mpOutFormat = av_guess_format("mp4", NULL, NULL);
    }
    if (!mpOutFormat) {
        AM_ERROR("No outFormat\n");
        return ME_ERROR;
    }

    //allocate the output media context
    if (!(mpFormat = avformat_alloc_context())) {
    	AM_ERROR("avformat_alloc_context error\n");
    	return ME_ERROR;
    }
    mpFormat->oformat = mpOutFormat;
    snprintf(mpFormat->filename, sizeof(mpFormat->filename), "%s", mpOutputFileName);

    memset(ap, 0, sizeof(*ap));
    if (av_set_parameters(mpFormat, ap) < 0) {
    	AM_ERROR("av_set_parameters error\n");
    	return ME_ERROR;
    }

    err = SetupVideoEnv();
    if(err != ME_OK){
        return err;
    }
    err = SetupAudioEnv();
    if(err != ME_OK)
        return err;

    //av_dump_format(mpFormat, 0, mpOutputFileName, 1);
    if (!(mpOutFormat->flags & AVFMT_NOFILE)) {
        if (avio_open(&mpFormat->pb, mpFormat->filename, AVIO_FLAG_WRITE) < 0) {
            AM_ERROR("Failed to open '%s'\n", mpFormat->filename);
            return ME_ERROR;
    	}
    }
    if (av_write_header(mpFormat) < 0) {
    	AM_ERROR("Failed to write header\n");
    	return ME_ERROR;
    }

    AM_INFO("SetupMuxerEnv Done\n");
    return ME_OK;
}

AM_ERR CGMuxerFFMpeg::SetupVideoEnv()
{
    AVCodecContext *video_enc;

    if(mpConfig->configVideo == AM_FALSE)
        return ME_OK;

    AM_INFO("pFormat->nb_streams = %d\n", mpFormat->nb_streams);
    if (!(mVideoStreamInfo.pStream = av_new_stream(mpFormat, mpFormat->nb_streams))) {
    	return ME_ERROR;
    }
    AM_INFO("pFormat->nb_streams = %d\n", mpFormat->nb_streams);

    mVideoStreamInfo.pStream->probe_data.filename = mpFormat->filename;
    avcodec_get_context_defaults2(mVideoStreamInfo.pStream->codec, AVMEDIA_TYPE_VIDEO);
    video_enc = mVideoStreamInfo.pStream->codec;
    if (mpFormat->oformat->flags & AVFMT_GLOBALHEADER)
    	video_enc->flags |= CODEC_FLAG_GLOBAL_HEADER;
    video_enc->codec_id = (CodecID)mpConfig->videoInfo.codec;
    video_enc->time_base= (AVRational){3003,90000};
    video_enc->width = mpConfig->videoInfo.width;
    video_enc->height = mpConfig->videoInfo.height;
    video_enc->sample_aspect_ratio = mVideoStreamInfo.pStream->sample_aspect_ratio = (AVRational) {1, 1};
    //video_enc->has_b_frames = 2;
    //
    video_enc->extradata_size = mpConfig->videoInfo.extrasize;
    video_enc->extradata = (uint8_t*)malloc(video_enc->extradata_size);//ffmpeg need to free this mem
    memcpy((void*)(video_enc->extradata), (void*)(mpConfig->videoInfo.extradata), video_enc->extradata_size);
    return ME_OK;
}

AM_ERR CGMuxerFFMpeg::SetupAudioEnv()
{
    AVCodecContext *audio_enc;
    AVCodec *codec;

    if(mpConfig->configAudio == AM_FALSE)
        return ME_OK;

    if (!(mAudioStreamInfo.pStream = av_new_stream(mpFormat, mpFormat->nb_streams))) return ME_ERROR;
    mAudioStreamInfo.pStream->probe_data.filename = mpFormat->filename;
    avcodec_get_context_defaults2(mAudioStreamInfo.pStream->codec, AVMEDIA_TYPE_AUDIO);
    audio_enc = mAudioStreamInfo.pStream->codec;
    if (mpFormat->oformat->flags & AVFMT_GLOBALHEADER)
    	audio_enc->flags |= CODEC_FLAG_GLOBAL_HEADER;
    audio_enc->codec_id = (CodecID)mpConfig->audioInfo.codec;
    audio_enc->bit_rate = mpConfig->audioInfo.bitrate;
    audio_enc->sample_rate = mpConfig->audioInfo.samplerate;
    audio_enc->channels = mpConfig->audioInfo.channels;
    audio_enc->sample_fmt = (AVSampleFormat)mpConfig->audioInfo.samplefmt;
    if (audio_enc->codec_id == CODEC_ID_AC3) {
    	if (audio_enc->channels == 1)
    		audio_enc->channel_layout = CH_LAYOUT_MONO;
    	else if (audio_enc->channels == 2)
    		audio_enc->channel_layout = CH_LAYOUT_STEREO;
    }
    //
    if (mpConfig->audioInfo.codec == CODEC_ID_AAC && AMBA_AAC ) {
    	/*************************************************
    	audioObjectType; 5 [2 AAC LC]
    	samplingFrequencyIndex; 4 [0x3 48000,0x8 16000,0xb 8000]
    	channelConfiguration; 4
    	frameLengthFlag;1
    	dependsOnCoreCoder; 1
    	extensionFlag; 1
    	**************************************************/
    	audio_enc->extradata_size = 2;
    	//Fix me, Hard code for AAC LC/48000/2/0/0/0
    	mAudioExtra[0] = 0x11;//0001 0001
    	mAudioExtra[1] = 0x90;//1001 0000
    	audio_enc->extradata = (uint8_t*)mAudioExtra;
    }
    if (!(codec = avcodec_find_encoder(audio_enc->codec_id))) {
    	AM_ERROR("Failed to find audio codec %d,\n",audio_enc->codec_id);
    	return ME_ERROR;
    }
    if (avcodec_open(audio_enc, codec) < 0) {
    	AM_ERROR("Failed to open audio codec\n");
    	return ME_ERROR;
    }

    return ME_OK;
}

AM_ERR CGMuxerFFMpeg::FinishMuxerEnv()
{
    AM_INFO("FinishMuxerEnv ....\n");
    if (av_write_trailer(mpFormat)<0) {
    	AM_ERROR(" av_write_trailer err\n");
    	return ME_ERROR;
    }
    AM_INFO("FinishMuxerEnv Done\n");
    return ME_OK;
}

void CGMuxerFFMpeg::Delete()
{
    AM_INFO("CGMuxerFFMpeg Delete\n");
    if(mState != STATE_PENDING){
        AM_INFO("Still Process Finish\n");
    }
    CMD cmd(CMD_STOP);
    PerformCmd(cmd, AM_TRUE);
    AM_INFO("CGMuxerFFMpeg CMD_STOP Done\n");
#if (PLATFORM_LINUX && TARGET_USE_AMBARELLA_I1_DSP)
    if (mbLoaderEnabled) {
        mpLoader->StopLoader();
        mpLoader->Delete();
        mpLoader = NULL;
        mbLoaderEnabled = AM_FALSE;
    }
#endif
    if(mpOutputFileName){
        free(mpOutputFileName);
        mpOutputFileName = NULL;
    }

    if(mpBaseFileName){
        free(mpBaseFileName);
        mpBaseFileName = NULL;
    }

    if(mpFileFormate){
        free(mpFileFormate);
        mpFileFormate = NULL;
    }

    ClearQueue(mpVideoQ);
    ClearQueue(mpAudioQ);

    AM_DELETE(mpVideoQ);
    AM_DELETE(mpAudioQ);
    AM_DELETE(mpWriter);
    inherited::Delete();
    AM_INFO("CGMuxerFFMpeg Delete Done\n");
}

AM_ERR CGMuxerFFMpeg::ClearQueue(CQueue* queue)
{
    AM_BOOL rval;
    CGBuffer buffer;
    while(1)
    {
        rval = queue->PeekData(&buffer, sizeof(CGBuffer));
        if(rval == AM_FALSE)
        {
            break;
        }
        //release this packet
        buffer.ReleaseContent();
    }
    return ME_OK;
}

CGMuxerFFMpeg::~CGMuxerFFMpeg()
{

}

AM_ERR CGMuxerFFMpeg::Initialize()
{
    AM_ERR err;

    err = SetupMuxerEnv();
    if(err != ME_OK)
        return err;

    mVideoStreamInfo.bAutoBoundaryReached = !mpConfig->configVideo;
    mVideoStreamInfo.bAutoBoundaryStarted = !mpConfig->configVideo;
    mVideoStreamInfo.bNextFileTimeThresholdSet = !mpConfig->configVideo;
    mVideoStreamInfo.CurrentFrameCount = 0;
    mVideoStreamInfo.EndPTS = 0;
    mVideoStreamInfo.StartPTS = 0;

    mAudioStreamInfo.bAutoBoundaryReached = !mpConfig->configAudio;
    mAudioStreamInfo.bAutoBoundaryStarted = !mpConfig->configAudio;
    mAudioStreamInfo.bNextFileTimeThresholdSet = !mpConfig->configAudio;
    mAudioStreamInfo.CurrentFrameCount = 0;
    mAudioStreamInfo.EndPTS = 0;
    mAudioStreamInfo.StartPTS = 0;

    if((!mbTranscodeStream) && mpConfig->videoInfo.width!=1920) InitMD();

    return err;
}

AM_ERR CGMuxerFFMpeg::InitMD()
{
    AM_ERR err = ME_OK;
    if(mbMDFileCreated==0){
        char name[128];
        //generate filename
        strcpy(name, mpOutputFileName);
        strcat(name, ".mdinfo");
        err = mpWriter->CreateFile(name);
        if(err != ME_OK){
            AM_ERROR("ProcessMD[%d]: CreateFile %s error!.\n", mIndex, name);
            mbMDFileCreated = -1;
            return err;
        }
        mbMDFileCreated = 1;

        time_t fileStartTime = time(NULL);
        struct tm *mpStartTime = localtime(&fileStartTime);
        if(mbMDFileCreated==1){
            char str[128];
            strftime(str, sizeof(str), "{\n\"begin\":\t\"%Y-%m-%d %H:%M:%S\",\n", mpStartTime);
            err = mpWriter->WriteFile(str, strlen(str));
        }
    }
    return err;
}

AM_ERR CGMuxerFFMpeg::FinalizeMD()
{
    AM_ERR err;

    char str[128];

    time_t stoptime = time(NULL);
    struct tm *mpStopTime = NULL;
    mpStopTime = localtime(&stoptime);

    if(mbMDFileCreated==2){
        strcpy(str, "\n],\n");
        err = mpWriter->WriteFile(str, strlen(str));
    }

    strftime(str, sizeof(str), "\"end\":\t\t\"%Y-%m-%d %H:%M:%S\"\n}\n", mpStopTime);
    err = mpWriter->WriteFile(str, strlen(str));

    mpWriter->CloseFile();
    mbMDFileCreated = 0;
    return err;
}

AM_ERR CGMuxerFFMpeg::Finalize()
{
    AM_ERR err;

    if(!mpFormat || !mpOutFormat){
        return ME_OK;
    }
    //getFileInformation();
    mFileDuration = mVideoStreamInfo.CurrentFrameCount * mFrameInterval;
    mpFormat->duration = (mFileDuration)/IParameters::TimeUnitDen_90khz;
    mpFormat->file_size = mCurrentTotalFilesize;

    if(mFileDuration){
        mFileBitrate = (AM_UINT)(((float)mCurrentTotalFilesize*8*IParameters::TimeUnitDen_90khz)/((float)mFileDuration));
    }

    AM_INFO("** start write trailer, duration %lld, size %lld, bitrate %u.\n", mpFormat->duration, mpFormat->file_size, mFileBitrate);
    err = FinishMuxerEnv();
    if(err != ME_OK){
        AM_ERROR("finish muxer failed\n");
        return ME_ERROR;
    }
    AM_INFO("** write trailer done.\n");

    //update filename

    //clear ffmpeg context
    ClearFFMpegContext();

    //clear some variables
    mCurrentTotalFilesize = 0;
    mFileDuration = 0;
    mFileBitrate = 0;
    mCurrentFileIndex++;

    if(mbMDFileCreated>0){
    err = FinalizeMD();
        if(err != ME_OK){
            AM_ERROR("FinalizeMD failed\n");
            return ME_ERROR;
        }
    }
#if (PLATFORM_LINUX && TARGET_USE_AMBARELLA_I1_DSP)
    if (mbLoaderEnabled == AM_TRUE) {
        AM_INFO("Loader endabled, upload file: %s to ALIYUN OSS!\n", mpOutputFileName);
        err = mpLoader->HandleObject(mpOutputFileName, (mCurrentFileIndex -1), 1);
        if (err != ME_OK) {
            AM_ERROR("HandleObject failed! file name: %s, index: %d, type: upload\n", mpOutputFileName, (mCurrentFileIndex -1));
        }
    }
#endif

    return ME_OK;
}

void CGMuxerFFMpeg::ClearFFMpegContext()
{
    if(mAudioStreamInfo.pStream){
        if(mAudioStreamInfo.pStream->codec){
            avcodec_close(mAudioStreamInfo.pStream->codec);
        }
        mAudioStreamInfo.pStream = NULL;
    }
    AM_INFO("ClearFFMpegContext 1\n");
    if(mVideoStreamInfo.pStream)
        mVideoStreamInfo.pStream = NULL;
    AM_INFO("ClearFFMpegContext 2\n");
    if(!(mpOutFormat->flags & AVFMT_NOFILE)){
        /* close the output file */
        avio_close(mpFormat->pb);
    }
    AM_INFO("ClearFFMpegContext 3\n");
    avformat_free_context(mpFormat);
    mpFormat = NULL;
    mpOutFormat = NULL;
    AM_INFO("ClearFFMpegContext done\n");
    return;
}

bool CGMuxerFFMpeg::IsCommingBufferAutoFileBoundary(CGBuffer* pBuffer)
{
    AM_ASSERT(pBuffer);
    AM_UINT type = pBuffer->GetStreamType();
    if(false == mVideoStreamInfo.bNextFileTimeThresholdSet || false == mAudioStreamInfo.bNextFileTimeThresholdSet)
        return false;

    if(type == mMasterStreamType && PredefinedPictureType_IDR == pBuffer->mFrameType){

        if(mSavingCondition == IParameters::MuxerSavingCondition_FrameCount){
            if((mVideoStreamInfo.CurrentFrameCount + 1) < mAutoSaveFrameCount){
                return false;
            }
            return true;
        }else if(mSavingCondition == IParameters::MuxerSavingCondition_InputPTS){
            if(pBuffer->GetPTS() < mVideoStreamInfo.NextFileTimeThreshold){
                return false;
            }
            AM_INFO("mVideoStreamInfo.NextFileTimeThreshold %llu, pBuffer->GetPTS() %llu\n", mVideoStreamInfo.NextFileTimeThreshold, pBuffer->GetPTS());
            return true;
        }else if(mSavingCondition == IParameters::MuxerSavingCondition_CalculatedPTS){
            //TODO
            //AM_U64 comming_pts = pBuffer->GetPTS() - mVideoStreamInfo.SessionInputPTSStartPoint + mVideoStreamInfo.SessionPTSStartPoint;
            //if(comming_pts < mVideoStreamInfo.NextFileTimeThreshold){
                return false;
            //}
            //return true;
        }
    }else if(type != mMasterStreamType && type != STREAM_VIDEO){
        if(!cutfile_with_precise_pts){
            return false;
        }
        if(pBuffer->GetPTS() < mAudioStreamInfo.NextFileTimeThreshold){
            return false;
        }
        return true;
    }
    return false;
}

//TODO
void CGMuxerFFMpeg::updateFileName(AM_UINT file_index)
{
#ifdef DEBUG_NVR_OVERNIGHT
    if(mMaxFileCount != 0){
        AM_INFO("NVR: overnight test!max file count is %d\n", mMaxFileCount);
        if(file_index >= mMaxFileCount){
            file_index = 0;
            mCurrentFileIndex = 0;
        }

        memset(mpOutputFileName, 0, mOutputFileNameLength);
        snprintf(mpOutputFileName, mOutputFileNameLength, "stream%d_%03d%s", mMuxerIndex, file_index, mpFileFormate);
        return;
    }
#endif
    if(mpBaseFileName &&(mpConfig->nameWithTime==AM_FALSE)){
        memset(mpOutputFileName, 0, mOutputFileNameLength);
        snprintf(mpOutputFileName, mOutputFileNameLength, "%s_%06d%s", mpBaseFileName, file_index, mpFileFormate);
        return;
    }
    char datetime_buffer[128];
    time_t mCurTime;
    struct tm *mpLocalTime = NULL;
    memset(datetime_buffer, 0, sizeof(datetime_buffer));
    memset(mpOutputFileName, 0, mOutputFileNameLength);

    mCurTime = time(NULL);
    mpLocalTime = localtime(&mCurTime);
    strftime(datetime_buffer, sizeof(datetime_buffer), "%Z%Y-%m-%d_[%H-%M]-", mpLocalTime);

    mCurTime += (time_t)mDuration;
    mpLocalTime = localtime(&mCurTime);
    strftime(datetime_buffer + strlen(datetime_buffer), sizeof(datetime_buffer) - strlen(datetime_buffer), "[%H-%M]",mpLocalTime);

    if(mpBaseFileName)
        snprintf(mpOutputFileName, mOutputFileNameLength, "%s_%03d_%s%s", mpBaseFileName, file_index, datetime_buffer,mpFileFormate);
    else
        snprintf(mpOutputFileName, mOutputFileNameLength, "%d_%03d_%s%s", mMuxerIndex, file_index, datetime_buffer,mpFileFormate);
    return;
}
//---------------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------------
AM_ERR CGMuxerFFMpeg::ConfigMe(CUintMuxerConfig* con)
{
    AM_ERR err = ME_OK;
    mpConfig = con;
    //parse file name and file formate
    AM_UINT name_length = strlen(mpConfig->fileName);
    if(name_length > 0){
        char* point = strrchr(mpConfig->fileName, '.');
        memset(mpFileFormate, 0, 10);
        if(point){
            mpBaseFileName = (char*)malloc(name_length + 1);
            memset(mpBaseFileName, 0, name_length + 1);
            strncpy(mpBaseFileName, mpConfig->fileName, name_length - strlen(point));
            strncpy(mpFileFormate, point, 9);
        }else{
            mpBaseFileName = (char*)malloc(name_length + 1);
            memset(mpBaseFileName, 0, name_length + 1);
            strncpy(mpFileFormate, ".ts", 9);
            strncpy(mpBaseFileName, mpConfig->fileName, name_length);
        }
    }
    avcodec_init();
    av_register_all();
    SendCmd(CMD_RUN);//muxer enter onRun
    AM_INFO("ConfigMe Done:%d\n", err);
    return err;
}

AM_ERR CGMuxerFFMpeg::UpdateConfig(CUintMuxerConfig* con)
{
    return ME_OK;
}

AM_ERR CGMuxerFFMpeg::PerformCmd(CMD& cmd, AM_BOOL isSend)
{
    AM_ERR err;
    if(isSend == AM_TRUE)
    {
        err = MsgQ()->SendMsg(&cmd, sizeof(cmd));
    }else{
        err = MsgQ()->PostMsg(&cmd, sizeof(cmd));
    }
    return err;
}

AM_ERR CGMuxerFFMpeg::FeedData(CGBuffer* buffer)
{
    STREAM_TYPE type = buffer->GetStreamType();
    if(type != STREAM_VIDEO && type != STREAM_AUDIO){
        AM_ASSERT(0);
        buffer->ReleaseContent();
        return ME_BAD_FORMAT;
    }

    CQueue* mpQ = (type == STREAM_VIDEO) ? mpVideoQ : mpAudioQ;
    if(mpQ->GetDataCnt() < MUXER_MAX_BUFFER_VIDEO_DATA){
        mpQ->PutData(buffer, sizeof(CGBuffer));
        return ME_OK;
    }

    if(mbFlowFull == AM_FALSE){
        AM_INFO("Video Save too Slowly!\n");
        CMD cmd(CMD_FULL);
        //save this buffer ptr or will crash by up-level because this is not a sendcmd
        AM_ASSERT(mBufferDump.GetBufferType() == NOINITED_BUFFER);
        mBufferDump = *buffer;
        //cmd.pExtra = buffer;
        mbFlowFull = AM_TRUE;
        PerformCmd(cmd, AM_FALSE);
    }else{
        buffer->ReleaseContent();
    }

    return ME_OK;
}

AM_ERR CGMuxerFFMpeg::FinishMuxer()
{
    CMD cmd(CMD_FINISH);
    //send this cmd?
    PerformCmd(cmd, AM_TRUE);
    AM_INFO("FinishMuxer Done\n");
    return ME_OK;
}

AM_ERR CGMuxerFFMpeg::QueryInfo(AM_INT type, CParam& par)
{
    return ME_OK;
}

AM_ERR CGMuxerFFMpeg::Dump()
{
    return ME_OK;
}

AM_ERR CGMuxerFFMpeg::postMDMsg(AM_INT msg)
{
    CMD cmd(CMD_MD_EVENT);
    cmd.flag = msg;
    PerformCmd(cmd, AM_FALSE);
    AM_INFO("postMDMsg %d Done\n", msg);
    return ME_OK;
}

void CGMuxerFFMpeg::OnRun()
{
    AM_ERR err;
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
//    AM_INT ret = 0;

    mbRun = true;
    CmdAck(ME_OK);
    mState = STATE_IDLE;

    //saving file's variables
    CQueue* tmp_queue = NULL;
    am_pts_t new_time_threshold = 0;
    AM_UINT stream_type = STREAM_NULL;
    AM_UINT wait_stream_type = STREAM_NULL;

    if(IParameters::MuxerSavingFileStrategy_AutoSeparateFile == mSavingFileStrategy){
        mFrameInterval = ((AM_U64)IParameters::TimeUnitDen_90khz)*mpConfig->framerate_den/mpConfig->framerate_num;
        mIDRFrameInterval = mFrameInterval*mIDRFrameCountInterval;
        AM_INFO("muxer %d, before enter onRun, mFrameInterval %u, mIDRFrameInterval %u\n", mMuxerIndex, mFrameInterval, mIDRFrameInterval);
    }

    updateFileName(mCurrentFileIndex);
    err = Initialize();
    if(err != ME_OK){
        mState = STATE_ERROR;
        AM_ERROR("muxer: %d, initialize error!\n", mMuxerIndex);
    }

    AM_INFO("CGMuxerFFMpeg OnRun Enter.\n");
    while(mbRun)
    {
        switch(mState)
        {
        case STATE_IDLE:
            type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),  &result);
            if(type == CQueue::Q_MSG){
                ProcessCmd(cmd);
            }else{
                CQueue* resultQ = result.pDataQ;
                if(!resultQ->PeekData(&mBuffer, sizeof(CGBuffer))){
                    AM_ERROR("!!PeekData Failed!\n");
                    break;
                }
                stream_type =  mBuffer.GetStreamType();
                if(IParameters::MuxerSavingFileStrategy_AutoSeparateFile == mSavingFileStrategy){
                    if(true == IsCommingBufferAutoFileBoundary(&mBuffer)){
                        if(!cutfile_with_precise_pts){//don't care audio
                            if(mMasterStreamType == stream_type){
                                //video
                                AM_ASSERT(false == mVideoStreamInfo.bCachedBuffer);
                                AM_ASSERT(mVideoStreamInfo.bCachedBuffer == false);
                                new_time_threshold = mBuffer.GetPTS();
                                AMLOG_INFO("[Muxer %d], detected AUTO saving file, new video IDR threshold %llu, NextFileTimeThreshold %llu, diff (%lld).\n",
                                    mMuxerIndex, new_time_threshold, mVideoStreamInfo.NextFileTimeThreshold, new_time_threshold - mVideoStreamInfo.NextFileTimeThreshold);
                                mVideoStreamInfo.bAutoBoundaryReached = true;

                                memcpy((void*)&(mVideoStreamInfo.CachedBuffer), (void*)&mBuffer, sizeof(CGBuffer));
                                mVideoStreamInfo.bCachedBuffer = true;
                                mState = STATE_SAVING_PARTIAL_FILE;
                            }else{
                                //audio
                                ProcessData(&mBuffer);
                                mState = STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_MASTER_PIN;
                                mAudioStreamInfo.bAutoBoundaryReached = true;
                            }
                        }else{
                            if(mMasterStreamType == stream_type){
                                AM_ASSERT(PredefinedPictureType_IDR ==mBuffer.mFrameType);
                                AM_ASSERT(mVideoStreamInfo.bCachedBuffer == false);
                                new_time_threshold = mBuffer.GetPTS();
                                AMLOG_INFO("[Muxer %d], detected AUTO saving file, new video IDR threshold %llu, Video NextFileTimeThreshold %llu, diff (%lld).\n",
                                    mMuxerIndex, new_time_threshold, mVideoStreamInfo.NextFileTimeThreshold, new_time_threshold - mVideoStreamInfo.NextFileTimeThreshold);
                                mVideoStreamInfo.bAutoBoundaryReached = true;
                                memcpy((void*)&(mVideoStreamInfo.CachedBuffer), (void*)&mBuffer, sizeof(CGBuffer));
                                mVideoStreamInfo.bCachedBuffer = true;
                                mState = STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN;
                            }else{
                                new_time_threshold = mVideoStreamInfo.NextFileTimeThreshold;
                                AMLOG_INFO("[Muxer %d], detected AUTO saving file, non-master pin detect boundary, pts %llu, Audio NextFileTimeThreshold %llu, diff %lld, write buffer here.\n",
                                    mMuxerIndex, mBuffer.GetPTS(), mAudioStreamInfo.NextFileTimeThreshold, mBuffer.GetPTS() - mAudioStreamInfo.NextFileTimeThreshold);
                                ProcessData(&mBuffer);
                                mAudioStreamInfo.bAutoBoundaryReached = true;
                                mState = STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_MASTER_PIN;
                            }
                        }
                        break;
                    }
                }
                err = ProcessData(&mBuffer);
            }
            break;

        case STATE_SAVING_PARTIAL_FILE:
            AM_ASSERT(new_time_threshold >= mVideoStreamInfo.NextFileTimeThreshold);
            AM_ASSERT(mVideoStreamInfo.bAutoBoundaryReached == true);

            //write remainning packet
            AMLOG_INFO("[Muxer %d] start saving patial file, peek all packet if PTS less than threshold.\n", mMuxerIndex);

            if(!mAudioStreamInfo.bAutoBoundaryReached){
                while(true == mpAudioQ->PeekData(&mBuffer, sizeof(CGBuffer))){
                    AM_ASSERT(mBuffer.GetStreamType() == STREAM_AUDIO);
                    ProcessData(&mBuffer);
                    if(mBuffer.GetPTS() > new_time_threshold){
                        break;
                    }
                }
            }
            AM_INFO("[Muxer %d] finalize current file, %s.\n", mMuxerIndex, mpOutputFileName);
            err = Finalize();
            if(err != ME_OK){
                AM_ERROR("CGMuxerFFMpeg::Finalize fail, enter error state.\n");
                mState = STATE_ERROR;
                break;
            }

            updateFileName(mCurrentFileIndex);

            AM_INFO("[Muxer %d] initialize new file %s.\n", mMuxerIndex, mpOutputFileName);
            err = Initialize();
            if(err != ME_OK){
                AM_ERROR("CFFMpegMuxer::Initialize fail, enter error state.\n");
                mState = STATE_ERROR;
                break;
            }
            AM_INFO("[Muxer %d] end saving patial file.\n", mMuxerIndex);
            mState = STATE_IDLE;
            break;

        case STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_MASTER_PIN:
            AM_ASSERT(new_time_threshold >= mVideoStreamInfo.NextFileTimeThreshold);
            AM_ASSERT(mVideoStreamInfo.bAutoBoundaryReached == false);

            AMLOG_INFO("Muxer %d, [avsync]: PEEK buffers in master pin start.\n", mMuxerIndex);

            while(true == mpVideoQ->PeekData(&mBuffer, sizeof(CGBuffer))){
                if((mBuffer.GetPTS() > mVideoStreamInfo.NextFileTimeThreshold) && (PredefinedPictureType_IDR == mBuffer.mFrameType)){
                    AM_ASSERT(mVideoStreamInfo.bCachedBuffer == false);
                    mVideoStreamInfo.CachedBuffer.ReleaseContent();
                    memcpy((void*)&(mVideoStreamInfo.CachedBuffer), (void*)&mBuffer, sizeof(CGBuffer));
                    mVideoStreamInfo.bCachedBuffer = true;

                    new_time_threshold = mBuffer.GetPTS();
                    AMLOG_INFO("get IDR boundary, pts %llu, Video NextFileTimeThreshold %llu, diff %lld.\n", new_time_threshold, mVideoStreamInfo.NextFileTimeThreshold, new_time_threshold - mVideoStreamInfo.NextFileTimeThreshold);
                    mVideoStreamInfo.bAutoBoundaryReached = true;
                    break;
                }
                err = ProcessData(&mBuffer);
            }
            if(mVideoStreamInfo.bAutoBoundaryReached == true){
                AMLOG_INFO("[avsync]: PEEK buffers in master pin done, go to non-master pin.\n");
                mState = STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN;
            }else{
                AMLOG_INFO("[avsync]: PEEK buffers in master pin not finished, go to wait master boundary(IDR).\n");
                wait_stream_type = STREAM_VIDEO;
                mState = STATE_SAVING_PARTIAL_FILE_WAIT_ONE_PIN;
            }
            break;

        case STATE_SAVING_PARTIAL_FILE_WAIT_ONE_PIN:
            AMLOG_INFO("[avsync]: WAIT which Queue?  %d.\n", wait_stream_type);
            AM_ASSERT((AM_INT)wait_stream_type != -1);
            AM_ASSERT(false == mVideoStreamInfo.bAutoBoundaryReached ||false == mAudioStreamInfo.bAutoBoundaryReached);
            if(wait_stream_type == STREAM_VIDEO && mVideoStreamInfo.bCachedBuffer == true){
                AMLOG_WARN("Already have get next IDR, goto next stage.\n");
                mVideoStreamInfo.bAutoBoundaryReached = true;
                mState = STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN;
                break;
            }

            if(wait_stream_type == STREAM_VIDEO){
                tmp_queue = mpVideoQ;
            }else if(wait_stream_type == STREAM_AUDIO){
                tmp_queue = mpAudioQ;
            }else{
                AM_ERROR("!!which queue to wait? %d\n", wait_stream_type);
                wait_stream_type = STREAM_NULL;
                mState = STATE_SAVING_PARTIAL_FILE;
                break;
            }

            type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), tmp_queue);
            if(type == CQueue::Q_MSG){
                ProcessCmd(cmd);
            }else{
                mState = (wait_stream_type == STREAM_AUDIO)?
                    STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN : STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_MASTER_PIN;
                wait_stream_type = STREAM_NULL;
            }
            //wait_stream_type = STREAM_NULL;
            break;

        case STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN:
            AM_ASSERT(new_time_threshold >= mVideoStreamInfo.NextFileTimeThreshold);
            AM_ASSERT(true  == mVideoStreamInfo.bAutoBoundaryReached);

            if(true == mAudioStreamInfo.bAutoBoundaryReached){
                AMLOG_INFO("muxer %d, all stream reache boundary.\n", mMuxerIndex);
                mState = STATE_SAVING_PARTIAL_FILE;
                break;
            }

            AMLOG_INFO("[avsync]: PEEK non-master pin(%p) start.\n", this);
            while(true == mpAudioQ->PeekData(&mBuffer, sizeof(CGBuffer))){
                err = ProcessData(&mBuffer);
                if(mBuffer.GetPTS() >= mAudioStreamInfo.NextFileTimeThreshold){
                    mAudioStreamInfo.bAutoBoundaryReached = true;
                    break;
                }
            }
            if(false == mAudioStreamInfo.bAutoBoundaryReached){
                AMLOG_INFO("[avsync]: non-master pin need wait.\n");
                wait_stream_type = STREAM_AUDIO;
                mState = STATE_SAVING_PARTIAL_FILE_WAIT_ONE_PIN;
            }
            break;

        case STATE_PENDING:
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            ProcessCmd(cmd);
            break;

        default:
            AM_ERROR("Check Me.\n");
            mbRun = false;
            break;
        }
    }
    AM_INFO("CGMuxerFFMpeg OnRun Exit.\n");
}

AM_ERR CGMuxerFFMpeg::DoStop()
{
    AM_ASSERT(mState == STATE_PENDING);
    return ME_OK;
}

AM_ERR CGMuxerFFMpeg::DoFinish()
{
    //HANDLE FINISH
    //FinishMuxerEnv();
    AM_ERR err;
    err = Finalize();
    if(err != ME_OK){
        AM_ERROR("muxer %d, DoFinish Finalize error!\n", mMuxerIndex);
    }

    AM_INFO("Save %s Done, Save Size:%d, SaveFrame:%d\n",
    mpConfig->fileName, mInfo.saveSize, mInfo.saveFrame);
    mState = STATE_PENDING;
    return ME_OK;
}

AM_ERR CGMuxerFFMpeg::DoFull(CMD& cmd)
{
    AM_INFO("DoFull\n");
    CGBuffer saveBuffer;
    //CGBuffer* buffer = (CGBuffer* )(cmd.pExtra);

    CQueue* mpQ = (mBufferDump.GetStreamType() == STREAM_VIDEO) ? mpVideoQ : mpAudioQ;
    AM_INT max = (mpQ == mpVideoQ) ? MUXER_MAX_BUFFER_VIDEO_DATA : MUXER_MAX_BUFFER_AUDIO_DATA;
    if(mpQ->GetDataCnt() < (AM_UINT)(max - 10)){
        //thread diff;
    }else{
        while(mpQ->GetDataCnt() >= (AM_UINT)(max -20)){
            AM_INFO("ReleaseContent\n");
            mpQ->PeekData(&saveBuffer, sizeof(CGBuffer));
            saveBuffer.ReleaseContent();
        }
    }

    mpQ->PutData(&mBufferDump, sizeof(CGBuffer));
    mBufferDump.Clear();
    mbFlowFull = AM_FALSE;
    return ME_OK;
}

AM_ERR CGMuxerFFMpeg::DogetMD(CMD& cmd)
{
    AM_ERR err;
    AM_INFO("ProcessMD %d.\n", cmd.flag);

    do{
        if(mbMDFileCreated<0){
            return ME_ERROR;
        }else if(mbMDFileCreated==0){
            InitMD();
        }
    }while(mbMDFileCreated<=0);

    {
        char str[128];
        if(mbMDFileCreated==1){
            char str[16] = "\"MD_Event\":[";
            err = mpWriter->WriteFile(str, strlen(str));
        }
        sprintf(str, "%s{\"type\":\t\"%s\" ,\t", (mbMDFileCreated==1)?"\n":",\n", (cmd.flag==0)?"start":"stop");
        err = mpWriter->WriteFile(str, strlen(str));
        if(mbMDFileCreated==1) mbMDFileCreated = 2;
        //generate string
        time_t mCurTime;
        struct tm *mpLocalTime = NULL;
        mCurTime = time(NULL);
        mpLocalTime = localtime(&mCurTime);
        strftime(str, sizeof(str), "\"date\": \"%Y-%m-%d\" ,\t\"time\": \"%H:%M:%S\"}", mpLocalTime);
        err = mpWriter->WriteFile(str, strlen(str));
    }

    return err;
}

AM_ERR CGMuxerFFMpeg::ProcessCmd(CMD& cmd)
{
    //AM_INFO("CGMuxerFFMpeg::ProcessCmd %d\n ", cmd.code);
//    AM_ERR err;
    //AM_U64 par;
    switch (cmd.code)
    {
    case CMD_STOP:
        DoFinish();
        DoStop();
        mbRun = false;
        CmdAck(ME_OK);
        break;

    case CMD_FINISH:
        DoFinish();
        CmdAck(ME_OK);
        break;

    case CMD_FULL:
        DoFull(cmd);
        break;

    case CMD_GOON:
        break;

    case CMD_MD_EVENT:
        DogetMD(cmd);
        break;

    default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }

    return ME_OK;
}

AM_ERR CGMuxerFFMpeg::ProcessData(CGBuffer* pBuffer)
{
    AM_ERR err = ME_OK;

    if(pBuffer->GetStreamType() == STREAM_VIDEO){
        if(mVideoStreamInfo.bCachedBuffer == true){
            //write cached buffer firstly
            err = ProcessVideoData(&(mVideoStreamInfo.CachedBuffer));
            //mVideoStreamInfo.CachedBuffer.ReleaseContent();
            mVideoStreamInfo.bCachedBuffer = false;
        }
        err = ProcessVideoData(pBuffer);
    }else{
        err = ProcessAudioData(pBuffer);
    }

    //handle save issue
    if(err != ME_OK){
        AM_ERROR("IO Wrong!!!!\n");
        AM_MSG msg;
        msg.code = IGMuxer::MUXER_MSG_ERROR;
        if (mpManager) mpManager->NotifyFromMuxer(mIndex, msg);
        mState = STATE_PENDING;
    }

    return err;
}

AM_ERR CGMuxerFFMpeg::ProcessVideoData(CGBuffer* pBuffer)
{
    //AM_INFO("ProcessVideoData ....\n ");
    //static AM_U64 lastDts = 0;
    AM_U8* dataPrt = pBuffer->PureDataPtr();
    AM_UINT dataSize = pBuffer->PureDataSize();
    //AM_INFO("Dump Muxer:");
    //AM_INFO("Packet size:%d, Data:(%02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x)\n", dataSize, dataPrt[0],dataPrt[1],dataPrt[2],dataPrt[3],dataPrt[4],dataPrt[5],dataPrt[6],
    //  dataPrt[7],dataPrt[8],dataPrt[9]);

    AM_ERR err = ME_OK;
    AM_INT ret = 0;
    AVPacket packet;
    AVPacket* bufferPkt = (AVPacket* )(pBuffer->GetExtraPtr());

    if(mbTranscodeStream) mVideoPts = pBuffer->GetPTS();

    av_init_packet(&packet);
    packet.stream_index = 0;
    packet.size = dataSize;
    packet.data = dataPrt;
    packet.pts = mVideoPts;
    packet.dts = AV_NOPTS_VALUE;
    //packet.pts = pBuffer->GetPTS();
    AMLOG_PTS("CGMuxerFFMpeg::ProcessVideoData: Video pts: %llu.\n", packet.pts);

    if((bufferPkt && (bufferPkt->flags & AV_PKT_FLAG_KEY)) || pBuffer->mFrameType == PredefinedPictureType_IDR){
        packet.flags |= AV_PKT_FLAG_KEY;
        mIDR = 1;
    }

    if(mIDR != 1){
        pBuffer->ReleaseContent();
        return ME_OK;
    }

    if(mbIDRFrameCountIntervalConfirmed == false && IParameters::MuxerSavingFileStrategy_AutoSeparateFile == mSavingFileStrategy){
        //re-caculate mIDRFrameCountInterval
        if(pBuffer->mFrameType == PredefinedPictureType_IDR){
            AM_U64 IDRFramePTSTwo = pBuffer->GetPTS();
            if(mIDRFramePTSOne == 0 || IDRFramePTSTwo <= mIDRFramePTSOne){
                mIDRFramePTSOne = IDRFramePTSTwo;
            }else{
                mIDRFrameCountInterval = (IDRFramePTSTwo - mIDRFramePTSOne)/(AM_U64)mFrameInterval;
                //TODO.maybe miss frame
                mIDRFrameCountInterval = (mIDRFrameCountInterval%10)? (mIDRFrameCountInterval + 10 - mIDRFrameCountInterval%10) : mIDRFrameCountInterval;
                mIDRFrameInterval = mIDRFrameCountInterval * mFrameInterval;
                AM_ASSERT(mIDRFrameCountInterval <= 60);
                AMLOG_INFO("muxer %d, get new mIDRFrameCountInterval %u\n", mMuxerIndex, mIDRFrameCountInterval);
                mbIDRFrameCountIntervalConfirmed = true;
            }
        }
    }

    if(false == mVideoStreamInfo.bStreamStart){
        mVideoStreamInfo.bStreamStart = true;
        mVideoStreamInfo.FirstPTS = mVideoPts = pBuffer->GetPTS();//the video stream's first pts
        packet.pts = mVideoPts;
        AMLOG_INFO("muxer %d, Get first Video PTS %llu.\n", mMuxerIndex, mVideoStreamInfo.FirstPTS);
    }

    if(mVideoStreamInfo.CurrentFrameCount == 0){
        mVideoStreamInfo.StartPTS = pBuffer->GetPTS();
        mVideoStreamInfo.bAutoBoundaryStarted = true;
        AMLOG_INFO("muxer %d, [cut file, boundary start(video), pts %llu]\n", mMuxerIndex, mVideoStreamInfo.StartPTS);
    }

    mVideoStreamInfo.EndPTS = pBuffer->GetPTS();
    mVideoStreamInfo.LastPTS = mVideoStreamInfo.EndPTS;

    if(false == mVideoStreamInfo.bNextFileTimeThresholdSet && IParameters::MuxerSavingFileStrategy_AutoSeparateFile == mSavingFileStrategy){
        AM_ASSERT(pBuffer->mFrameType == PredefinedPictureType_IDR);

        if(mbSavingTimeDurationChanged == true){
            mAutoSavingTimeDuration = ((mAutoSavingTimeDuration + mFrameInterval/2) /mFrameInterval)* mFrameInterval;
            AMLOG_INFO("muxer %d, after modification mAutoSavingTimeDuration is %llu.\n", mMuxerIndex, mAutoSavingTimeDuration);
            mbSavingTimeDurationChanged = false;
        }

        if(IParameters::MuxerSavingCondition_InputPTS == mSavingCondition){
            mVideoStreamInfo.NextFileTimeThreshold = mVideoStreamInfo.StartPTS + mAutoSavingTimeDuration;
            AM_U64 threshold_diff = mVideoStreamInfo.NextFileTimeThreshold % mIDRFrameInterval;
            if(threshold_diff == 0){
                mAudioStreamInfo.NextFileTimeThreshold = mVideoStreamInfo.NextFileTimeThreshold - mVideoStreamInfo.FirstPTS;
            }else{
                mAudioStreamInfo.NextFileTimeThreshold = mVideoStreamInfo.NextFileTimeThreshold - mVideoStreamInfo.FirstPTS - threshold_diff + mIDRFrameInterval;
            }
        }else if(IParameters::MuxerSavingCondition_CalculatedPTS == mSavingCondition){
            //TODO
            mVideoStreamInfo.NextFileTimeThreshold = mAutoSavingTimeDuration;
            mAudioStreamInfo.NextFileTimeThreshold = mAutoSavingTimeDuration;
        }
        mVideoStreamInfo.bNextFileTimeThresholdSet = true;
        mLastFramePTS = pBuffer->GetPTS();
        AMLOG_INFO("muxer %d, mVideoStreamInfo.NextFileTimeThreshold %llu\n", mMuxerIndex, mVideoStreamInfo.NextFileTimeThreshold);
    }else if(mVideoStreamInfo.bNextFileTimeThresholdSet == true){
        AM_U64 curPts = pBuffer->GetPTS();
        if(curPts > mLastFramePTS){
            if((curPts - mLastFramePTS) > MAX_PTS_DIFF){
                AMLOG_INFO("muxer %d, last %llu, current %llu\n", mMuxerIndex, mLastFramePTS, curPts);
                mVideoStreamInfo.NextFileTimeThreshold += curPts - mLastFramePTS;
                mAudioStreamInfo.NextFileTimeThreshold += curPts - mLastFramePTS;
                AMLOG_INFO("muxer %d, PTS Jump, new video threshold %llu, new audio threshold %llu\n", mMuxerIndex, mVideoStreamInfo.NextFileTimeThreshold, mAudioStreamInfo.NextFileTimeThreshold);
            }
            mLastFramePTS = curPts;
        }else{
            AM_ERROR("muxer %d, video pts jump backword?! last %llu, current %llu, check me!!\n", mMuxerIndex, mLastFramePTS, curPts);
        }
    }

    if(mNeedParseSPS == AM_TRUE)
    {
    	if(0)//I-frame
         {
            AM_INFO("Beginning to calculate sps-pps's length.\n");
            //mVideoExtraLen = GetSpsPpsLen(pBuffer->GetDataPtr ());
            AM_INFO("Calculate sps-pps's length completely.\n");
            //memcpy(mVideoExtra, packet.data, mVideoExtraLen + 8);
    	}
    }
    ///AM_U8* mData = new AM_U8[dataSize];
    //mData[0] = 0;
    //mData[1] = 0;
    //mData[2] = 0;
    //mData[3] = 1;
    //memcpy(mData, dataPrt, dataSize);
    //packet.data = mData;
    //packet.size = dataSize+4;

    if ((ret = av_write_frame(mpFormat, &packet)) != 0) {
        AM_ERROR("ProcessVideoData error!\n");
        err = ME_ERROR;
    }

    mVideoStreamInfo.CurrentFrameCount++;
    mCurrentTotalFilesize += dataSize;

    if(!mbTranscodeStream) mVideoPts += 3003;
    //delete[] mData;
    mInfo.saveSize += dataSize;
    mInfo.saveFrame++;
    pBuffer->ReleaseContent();
    return err;
}

AM_ERR CGMuxerFFMpeg::ProcessAudioData(CGBuffer* pBuffer)
{
    //AM_INFO("ProcessAudioData ....\n ");
    AM_ERR err = ME_OK;
    AM_INT ret = 0;
    AM_U8* dataPrt = pBuffer->PureDataPtr();
    AM_UINT dataSize = pBuffer->PureDataSize();
    AVPacket packet;

    av_init_packet(&packet);
    AM_UINT header_length = 0;

    if (mpConfig->audioInfo.codec == CODEC_ID_AAC && AMBA_AAC ) {          // skip ADTS header
/*ISO 13818-7
	adts_fixed_header()
	{
		syncword; 12 bslbf
		ID; 1 bslbf
		layer; 2 uimsbf
		protection_absent; 1 bslbf
		profile; 2 uimsbf
		sampling_frequency_index; 4 uimsbf
		private_bit; 1 bslbf
		channel_configuration; 3 uimsbf
		original/copy; 1 bslbf
		home; 1 bslbf
	}
	adts_variable_header()
	{
		copyright_identification_bit; 1 bslbf
		copyright_identification_start; 1 bslbf
		frame_length; 13 bslbf
		adts_buffer_fullness; 11 bslbf
		number_of_raw_data_blocks_in_frame; 2 uimsfb
	}
*/
    	AM_INT syncword;
	AM_U8 bit_pos = 0;
        AM_U8 *pAdts_header = dataPrt;
    	pAdts_header += ReadBit (pAdts_header, &syncword, &bit_pos, 12);
    	if (syncword != 0xFFF) {
            AM_ERROR("syncword != 0xFFFn");
    	}

    	bit_pos += 3;
    	AM_INT protection_absent;
    	pAdts_header += ReadBit (pAdts_header, &protection_absent, &bit_pos);
    	pAdts_header = dataPrt + 6;
    	bit_pos = 6;

    	header_length = 7;
    	AM_INT number_of_raw_data_blocks_in_frame;
    	pAdts_header += ReadBit (pAdts_header, &number_of_raw_data_blocks_in_frame, &bit_pos, 2);
    	if (number_of_raw_data_blocks_in_frame == 0) {
            if (protection_absent == 0) {
                header_length += 2;
            }
    	}
    }

    if(false == mAudioStreamInfo.bStreamStart || false == mVideoStreamInfo.bStreamStart){
        mAudioStreamInfo.bStreamStart = true;
        mAudioStreamInfo.FirstPTS = pBuffer->GetPTS();
    }

    if(true == mbCheckAudioFirstPts && true == mVideoStreamInfo.bStreamStart){
        AM_U64 max = (mAudioStreamInfo.FirstPTS > mVideoStreamInfo.FirstPTS)? mAudioStreamInfo.FirstPTS : mVideoStreamInfo.FirstPTS;
        AM_U64 min = (mAudioStreamInfo.FirstPTS < mVideoStreamInfo.FirstPTS)? mAudioStreamInfo.FirstPTS : mVideoStreamInfo.FirstPTS;
        if((max - min) > (min>>1)){
            AMLOG_INFO("muxer %d, max %llu, min %llu, diff %llu, need to adjust audio first pts\n", mMuxerIndex, max, min, max-min);
            mAudioStreamInfo.FirstPTS = mVideoStreamInfo.FirstPTS;
        }
        AMLOG_INFO("muxer %d, Get first Audio PTS %llu.\n", mMuxerIndex, mAudioStreamInfo.FirstPTS);
        mbCheckAudioFirstPts = false;
    }

    if(mAudioStreamInfo.CurrentFrameCount == 0){
        mAudioStreamInfo.bAutoBoundaryStarted = true;
        mAudioStreamInfo.StartPTS = pBuffer->GetPTS();
        AMLOG_INFO("muxer %d, [cut file, boundary start(audio), pts %llu]\n", mMuxerIndex, mAudioStreamInfo.StartPTS);
    }

    mAudioStreamInfo.EndPTS = pBuffer->GetPTS();
    mAudioStreamInfo.LastPTS = mAudioStreamInfo.EndPTS;

    if(false== mAudioStreamInfo.bNextFileTimeThresholdSet && IParameters::MuxerSavingFileStrategy_AutoSeparateFile == mSavingFileStrategy){
        if(true == mVideoStreamInfo.bNextFileTimeThresholdSet){
            if(IParameters::MuxerSavingCondition_InputPTS == mSavingCondition){
                mAudioStreamInfo.NextFileTimeThreshold += mAudioStreamInfo.FirstPTS;
            }
            mAudioStreamInfo.bNextFileTimeThresholdSet = true;
            AMLOG_INFO("muxer %d, mAudioStreamInfo.NextFileTimeThreshold %llu\n",mMuxerIndex,mAudioStreamInfo.NextFileTimeThreshold);
        }
    }

    packet.stream_index = 1;
    packet.size = dataSize - header_length;
    packet.data = dataPrt+ header_length;
    // convert pts/dts from us domain to sample frequency domain
    packet.dts = packet.pts = pBuffer->GetPTS();
    //packet.dts = packet.pts = av_rescale_q(pBuffer->GetPTS(), (AVRational){1,1000000},
    //    mpFormat->streams[packet.stream_index]->time_base);
    AMLOG_PTS("CGMuxerFFMpeg::ProcessVideoData: Audio pts: %llu.\n", packet.pts);
    if ((ret = av_write_frame(mpFormat, &packet)) != 0) {
        err = ME_ERROR;
        AM_ERROR("OnAudioBuffer error! err=%d\n", err);
    }
    //AM_INFO("OnAudioBuffer done,size:%d,pts:%d [%d/%d]\n",packet.size,(int)packet.pts,
        //mpFormat->streams[packet.stream_index]->time_base.num, mpFormat->streams[packet.stream_index]->time_base.den);

    mAudioStreamInfo.CurrentFrameCount++;
    mCurrentTotalFilesize += packet.size;
    pBuffer->ReleaseContent();
    return ME_OK;
}

AM_UINT CGMuxerFFMpeg::ReadBit(AM_U8 *pBuffer, AM_INT *value, AM_U8 *bit_pos, AM_UINT num)
{
    *value = 0;
    AM_UINT i = 0, j;
    for (j = 0; j < num; j++) {
        if (*bit_pos == 8) {
            *bit_pos = 0;
            i++;
        }

    	if (*bit_pos == 0) {
            if (pBuffer[i] == 0x03  &&
                pBuffer[i - 1] == 0 &&
                pBuffer[i - 2] == 0)
                i++;
    	}

    	*value <<= 1;
    	*value += pBuffer[i] >> (7 - (*bit_pos)++) &0x1;
    }
    return i;
}

AM_ERR CGMuxerFFMpeg::SetSavingTimeDuration( AM_UINT duration, AM_UINT maxfilecount)
{
    if(duration == 0){
        AM_INFO("Don't auto separate file or Stop auto separate file??\n");
        mSavingFileStrategy = IParameters::MuxerSavingFileStrategy_ToTalFile;
        mSavingCondition = IParameters::MuxerSavingCondition_Invalid;
        mAutoSavingTimeDuration = duration * IParameters::TimeUnitDen_90khz;
    }else{
        AM_INFO("auto separate file, time duration %d s\n", duration);
        mSavingFileStrategy = IParameters::MuxerSavingFileStrategy_AutoSeparateFile;
        mSavingCondition = IParameters::MuxerSavingCondition_InputPTS;
        mAutoSavingTimeDuration = duration * IParameters::TimeUnitDen_90khz;
        mbSavingTimeDurationChanged = true;
    }
    mDuration = duration;
    mMaxFileCount = maxfilecount;
    return ME_OK;
}

AM_ERR CGMuxerFFMpeg::EnableLoader(AM_BOOL flag)
{
    if (mbLoaderEnabled == flag) {
        return ME_OK;
    }
#if (PLATFORM_LINUX && TARGET_USE_AMBARELLA_I1_DSP)
    if (flag == AM_TRUE) {
        mpLoader = CGeneralLoader::Create();
        if (!mpLoader) {
            AM_ERROR("failed to creat CGeneralLoader!\n");
            return ME_ERROR;
        }
    } else {
        mpLoader->StopLoader();
        mpLoader->Delete();
        mpLoader = NULL;
    }
#endif
    mbLoaderEnabled = flag;
    return ME_OK;
}

AM_ERR CGMuxerFFMpeg::ConfigLoader(char* path, char* m3u8name, char* host, int count)
{
    if (mbLoaderEnabled == AM_FALSE) {
        AM_ERROR("Now Loader is disabled!\n");
        return ME_ERROR;
    }
#if (PLATFORM_LINUX && TARGET_USE_AMBARELLA_I1_DSP)
    if (!path || !m3u8name || !host) {
        AM_ERROR("ConfigLoader: invaild parameters!\n");
        return ME_ERROR;
    }
    SLoaderConfig config;
    strncpy(config.pPath, path, sizeof(config.pPath));
    strncpy(config.pNameM3U8, m3u8name, sizeof(config.pNameM3U8));
    strncpy(config.pNameHost, host, sizeof(config.pNameHost));
    config.mDuration = mDuration;
    config.mObjectCount = count;
    mpLoader->ConfigLoader(config);
#endif
    return ME_OK;
}