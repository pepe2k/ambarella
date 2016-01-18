
/*
 * rtsp_demuxer.h
 *
 * History:
 *    2012/04/28 - [Zhi He] created file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __RTSP_DEMUXER_H__
#define __RTSP_DEMUXER_H__

class CRTSPDemuxer;
class CRTSPDemuxerOutput;

#define DMAX_RTSP_NUMBER_OUTPUT_PIN 2
#define DMAX_RTSP_STRING_LEN 512

//simple queue
typedef struct simple_node_s
{
    AM_UINT ctx;
    struct simple_node_s* p_pre;
    struct simple_node_s* p_next;
} simple_node_t;

typedef struct simple_queue_s
{
    AM_UINT max_cnt;
    AM_UINT current_cnt;
    simple_node_t head;

    simple_node_t* freelist;

    pthread_mutex_t mutex;
    pthread_cond_t cond_notfull;
    pthread_cond_t cond_notempty;

    AM_UINT (*getcnt)(struct simple_queue_s* thiz);
    void (*lock)(struct simple_queue_s* thiz);
    void (*unlock)(struct simple_queue_s* thiz);

    void (*enqueue)(struct simple_queue_s* thiz, AM_UINT ctx);
    AM_UINT (*dequeue)(struct simple_queue_s* thiz);
    AM_UINT (*peekqueue)(struct simple_queue_s* thiz, AM_UINT* ret);
} simple_queue_t;

//-----------------------------------------------------------------------
//
// CRTSPDemuxer
//
//-----------------------------------------------------------------------
class CRTSPDemuxer: public CActiveFilter, public IDemuxer
{
    typedef CActiveFilter inherited;
    friend class CRTSPDemuxerOutput;

public:
    static int ParseMedia(struct parse_file_s *pParseFile, struct parser_obj_s *pParser);

public:
    static IFilter* Create(IEngine *pEngine);

protected:
    CRTSPDemuxer(IEngine *pEngine):
        inherited(pEngine, "RTSPDemuxer"),
        mVideoIndex(-1),
        mAudioIndex(-1),
        mbEnableVideo(1),
        mbEnableAudio(1),
        mFilterIndex(0),
        mVideoWidth(0),
        mVideoHeight(0),
        mpVideoOutput(NULL),
        mpAudioOutput(NULL),
        mpVideoBP(NULL),
        mpAudioBP(NULL),
        mnOutputPin(0)
    {
        AM_UINT i = 0;
        for (i = 0; i < DMAX_RTSP_NUMBER_OUTPUT_PIN; i++) {
            mpOutput[i] = NULL;
        }

        mpFullServerUrl = NULL;
        mpServerAddr = NULL;
        mpItemName = NULL;

        mServerRTSPPort = 554;
        mRTSPSeq = 2;

        mAudioTrackID = 1;
        mVideoTrackID = 0;

        mSessionID = 0;

        mServerRTPAudioPort = 0;
        mServerRTCPAudioPort = 0;
        mServerRTPVideoPort = 0;
        mServerRTCPVideoPort = 0;

        mClientRTPAudioPort = 0;
        mClientRTCPAudioPort = 0;
        mClientRTPVideoPort = 0;
        mClientRTCPVideoPort = 0;

        mRTSPSocket = -1;
        mRTPAudioSocket = -1;
        mRTPVideoSocket = -1;
        mRTCPAudioSocket = -1;
        mRTCPVideoSocket = -1;

        memset(mRTSPBuffer, 0x0, sizeof(mRTSPBuffer));
    }
    AM_ERR Construct();
    virtual ~CRTSPDemuxer();

protected:
    virtual bool ProcessCmd(CMD& cmd, simple_queue_t* video_queue, simple_queue_t* audio_queue);
    static void* recievingDataThread(void* _param);

public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete();
    virtual void SetIndex(AM_INT index){mFilterIndex = index;};

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

    virtual void EnableSubtitle(bool enable) {AM_ERROR("not support here.\n");}
    virtual AM_ERR  A5SPlayMode(AM_UINT mode) {AM_ERROR("not support here.\n"); return ME_NO_IMPL;}
    virtual AM_ERR  A5SPlayNM(AM_INT start_n, AM_INT end_m) {AM_ERROR("not support here.\n"); return ME_NO_IMPL;}

private:
    void sendTeardown();
    void printBytes(AM_U8* p, AM_UINT size);
    bool SendEOS(CRTSPDemuxerOutput *pPin);

private:
    AM_U8 mVideoIndex;
    AM_U8 mAudioIndex;
    AM_U8 mbEnableVideo;
    AM_U8 mbEnableAudio;

    AM_UINT mFilterIndex;

    AM_INT mVideoWidth, mVideoHeight;

    CRTSPDemuxerOutput *mpVideoOutput;
    CRTSPDemuxerOutput *mpAudioOutput;

    CFixedBufferPool *mpVideoBP;
    CFixedBufferPool *mpAudioBP;

    CRTSPDemuxerOutput* mpOutput[DMAX_RTSP_NUMBER_OUTPUT_PIN];
    AM_UINT mnOutputPin;

//rtsp related
private:
    char* mpFullServerUrl;
    char* mpServerAddr;
    char* mpItemName;

    AM_U16 mServerRTSPPort;
    AM_U16 mRTSPSeq;

    AM_U16 mAudioTrackID;
    AM_U16 mVideoTrackID;

    AM_U64 mSessionID;

    AM_U16 mServerRTPAudioPort;
    AM_U16 mServerRTCPAudioPort;
    AM_U16 mServerRTPVideoPort;
    AM_U16 mServerRTCPVideoPort;

    AM_U16 mClientRTPAudioPort;
    AM_U16 mClientRTCPAudioPort;
    AM_U16 mClientRTPVideoPort;
    AM_U16 mClientRTCPVideoPort;

    AM_INT mRTSPSocket;
    AM_INT mRTPAudioSocket;
    AM_INT mRTPVideoSocket;
    AM_INT mRTCPAudioSocket;
    AM_INT mRTCPVideoSocket;

    IParameters::StreamFormat mVideoFormat;
    IParameters::StreamFormat mAudioFormat;
    AM_UINT mAudioChannelNumber;
    AM_UINT mAudioSampleRate;

    //rtp read thread
    AM_INT mVideoPipeFd[2];
    AM_INT mAudioPipeFd[2];

private:
    struct sockaddr_in mServerAddrIn;

    char mRTSPBuffer[DMAX_RTSP_STRING_LEN];
};

//-----------------------------------------------------------------------
//
// CRTSPDemuxerOutput
//
//-----------------------------------------------------------------------
class CRTSPDemuxerOutput: public COutputPin
{
    typedef COutputPin inherited;
    friend class CRTSPDemuxer;

public:
    static CRTSPDemuxerOutput* Create(CFilter *pFilter);

protected:
    CRTSPDemuxerOutput(CFilter *pFilter);
    AM_ERR Construct();
    virtual ~CRTSPDemuxerOutput();


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

