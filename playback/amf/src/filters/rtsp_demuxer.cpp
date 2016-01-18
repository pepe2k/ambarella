
/*
 * rtsp_demuxer.cpp
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

#define LOG_NDEBUG 0
#define LOG_TAG "rtsp_demuxer"
//#define AMDROID_DEBUG

//depend on socket.h
#include <sys/socket.h>
#include <arpa/inet.h>
#if PLATFORM_LINUX
#include <netinet/in.h>
#include <netdb.h>
#endif

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_util.h"
#include "am_net.h"

#include "am_pbif.h"
#include "pbif.h"
#include "engine_guids.h"
#include "filter_list.h"

#if PLATFORM_ANDROID
#include "sys/atomics.h"
#endif
#include "osal.h"

#include "rtsp_demuxer.h"

//strings for RTSP
static const char* _rtsp_option_request_fmt =
    "OPTIONS %s/%s RTSP/1.0\r\n"
    "CSeq: %d\r\n"
    "User-Agent: Ambarella iOne v20120428\r\n"
    ;

static const char* _rtsp_describe_request_fmt =
    "DESCRIBE %s/%s RTSP/1.0\r\n"
    "CSeq: %d\r\n"
    "User-Agent: Ambarella iOne v20120428\r\n"
    "Accept: application/sdp"
    ;

static const char* _rtsp_setup_request_fmt =
    "SETUP %s:%d/%s/trackID=%d RTSP/1.0\r\n"
    "CSeq: %d\r\n"
    "User-Agent: Ambarella iOne v20120428\r\n"
    "Transport: RTP/AVP;unicast;client_port=%d-%d\r\n"
    ;

static const char* _rtsp_setup_request_with_sessionid_fmt =
    "SETUP %s:%d/%s/trackID=%d RTSP/1.0\r\n"
    "CSeq: %d\r\n"
    "User-Agent: Ambarella iOne v20120428\r\n"
    "Transport: RTP/AVP;unicast;client_port=%d-%d\r\n"
    "Session: %llx\r\n"
    ;

static const char* _rtsp_play_request_fmt =
    "PLAY %s:%d/%s/ RTSP/1.0\r\n"
    "CSeq: %d\r\n"
    "User-Agent: Ambarella iOne v20120428\r\n"
    "Session: %llx\r\n"
    "Range: ntp=0.000-\r\n"
    ;

static const char* _rtsp_teardown_request_fmt =
    "PLAY %s:%d/%s/ RTSP/1.0\r\n"
    "CSeq: %d\r\n"
    "User-Agent: Ambarella iOne v20120428\r\n"
    "Session: %llx\r\n"
    ;

static AM_UINT _getcnt(simple_queue_t* thiz)
{
    AM_UINT cnt;
    AM_ASSERT(thiz);

    pthread_mutex_lock(&thiz->mutex);
    cnt = thiz->current_cnt;
    pthread_mutex_unlock(&thiz->mutex);

    return cnt;
}

static void _lock(simple_queue_t* thiz)
{
    AM_ASSERT(thiz);
    pthread_mutex_lock(&thiz->mutex);
}

static void _unlock(simple_queue_t* thiz)
{
    AM_ASSERT(thiz);
    pthread_mutex_unlock(&thiz->mutex);
}

static void _enqueue(simple_queue_t* thiz, AM_UINT ctx)
{
    simple_node_t* ptmp=NULL;
    AM_ASSERT(thiz);

    pthread_mutex_lock(&thiz->mutex);

    if (thiz->max_cnt) {
        while(thiz->current_cnt >= thiz->max_cnt) {
            pthread_cond_wait(&thiz->cond_notfull, &thiz->mutex);
        }
    }

    if(!thiz->freelist) {
        ptmp = (simple_node_t*)malloc(sizeof(simple_node_t));
    } else {
        ptmp = thiz->freelist;
        thiz->freelist = thiz->freelist->p_next;
    }

    ptmp->ctx = ctx;
    ptmp->p_pre = &thiz->head;
    ptmp->p_next = thiz->head.p_next;
    thiz->head.p_next->p_pre = ptmp;
    thiz->head.p_next = ptmp;
    thiz->current_cnt++;
    if (thiz->current_cnt < 2) {
        pthread_cond_signal(&thiz->cond_notempty);
    }

    pthread_mutex_unlock(&thiz->mutex);
}

static AM_UINT _dequeue(simple_queue_t* thiz)
{
    simple_node_t* ptmp=NULL;
    AM_UINT ctx;
    AM_ASSERT(thiz);

    pthread_mutex_lock(&thiz->mutex);

    while (thiz->current_cnt == 0) {
        pthread_cond_wait(&thiz->cond_notempty,&thiz->mutex);
    }

    ptmp = thiz->head.p_pre;
    if(ptmp == &thiz->head) {
        AM_ERROR("fatal error: simple_queue_t currupt in dequeue.\n");
        return 0;
    }

    ptmp->p_pre->p_next = &thiz->head;
    thiz->head.p_pre = ptmp->p_pre;
    thiz->current_cnt--;

    ptmp->p_next = thiz->freelist;
    thiz->freelist = ptmp;
    ctx=ptmp->ctx;

    if ((thiz->max_cnt) && ((thiz->current_cnt + 1) >= thiz->max_cnt)) {
        pthread_cond_signal(&thiz->cond_notfull);
    }

    pthread_mutex_unlock(&thiz->mutex);

    return ctx;
}

static AM_UINT _peekqueue(simple_queue_t* thiz, AM_UINT* ret)
{
    simple_node_t* ptmp=NULL;
    AM_ASSERT(thiz);

    pthread_mutex_lock(&thiz->mutex);

    if (thiz->current_cnt == 0) {
        *ret = 0;
        return 0;
    }

    ptmp = thiz->head.p_pre;
    if(ptmp == &thiz->head) {
        AM_ERROR("fatal error: simple_queue_t currupt in peekqueue.\n");
        return 0;
    }

    ptmp->p_pre->p_next = &thiz->head;
    thiz->head.p_pre = ptmp->p_pre;
    thiz->current_cnt--;

    ptmp->p_next = thiz->freelist;
    thiz->freelist = ptmp;
    *ret = ptmp->ctx;

    if ((thiz->max_cnt) && ((thiz->current_cnt + 1) >= thiz->max_cnt)) {
        pthread_cond_signal(&thiz->cond_notfull);
    }

    pthread_mutex_unlock(&thiz->mutex);

    return 1;
}


simple_queue_t* _create_simple_queue(AM_UINT num)
{
    simple_queue_t* thiz = (simple_queue_t*)malloc(sizeof(simple_queue_t));
    if(!thiz)
        return NULL;

    thiz->max_cnt=num;
    thiz->current_cnt=0;
    thiz->head.p_next=thiz->head.p_pre=&thiz->head;
    thiz->freelist=NULL;

    thiz->getcnt=_getcnt;
    thiz->lock=_lock;
    thiz->unlock=_unlock;
    thiz->enqueue=_enqueue;
    thiz->dequeue=_dequeue;
    thiz->peekqueue=_peekqueue;

    pthread_mutex_init(&thiz->mutex,NULL);
    pthread_cond_init(&thiz->cond_notempty,NULL);
    pthread_cond_init(&thiz->cond_notfull,NULL);
    return thiz;
}

void _destroy_simple_queue(simple_queue_t* thiz)
{
    simple_node_t* ptmp=NULL, *ptmp1=NULL;
    AM_ASSERT(thiz);

    ptmp1=thiz->head.p_next;
    while(ptmp1!=&thiz->head)
    {
        ptmp=ptmp1;
        ptmp1=ptmp1->p_next;
        free(ptmp);
    }
    while(thiz->freelist)
    {
        ptmp=thiz->freelist;
        thiz->freelist=thiz->freelist->p_next;
        free(ptmp);
    }
    pthread_mutex_destroy(&thiz->mutex);
    pthread_cond_destroy(&thiz->cond_notempty);
    pthread_cond_destroy(&thiz->cond_notfull);
    free(thiz);
}

//for simple communication
enum {
    CONTROL_CMD_QUIT = 0,
    CONTROL_CMD_BUFFER_NOTIFY,
    CONTROL_CMD_SPEEDUP,
};

enum {
    DATA_THREAD_STATE_READ_FIRST_RTP_PACKET = 0,
    DATA_THREAD_STATE_READ_REMANING_RTP_PACKET,
    DATA_THREAD_STATE_WAIT_OUTBUFFER,
    DATA_THREAD_STATE_SKIP_DATA,
    DATA_THREAD_STATE_ERROR,
};

typedef struct
{
    CRTSPDemuxer* thiz;
    CRTSPDemuxerOutput* outpin;
    CFixedBufferPool* bufferpool;
    AM_INT socket;
    struct sockaddr src_addr;
    AM_INT pipe_fd;

    IParameters::StreamFormat format;

    simple_queue_t* queue;
} SRTPThreadParams;

filter_entry g_rtsp_demuxer = {
    "rtsp_demuxer",
    CRTSPDemuxer::Create,
    CRTSPDemuxer::ParseMedia,
    NULL,
};

static char* _find_string(char* buffer, const char* target, AM_INT target_len)
{
    char* ptmp = buffer;
    char* ptmp_end = buffer - target_len;
    AM_ASSERT(buffer);
    AM_ASSERT(target);
    AM_ASSERT(target_len);

    while (ptmp < ptmp_end) {
        if (!strncmp(ptmp, target, target_len)) {
            return ptmp;
        }
        ptmp ++;
    }
    return NULL;
}

IFilter* CreateRTSPDemuxer(IEngine *pEngine)
{
    return CRTSPDemuxer::Create(pEngine);
}

void CRTSPDemuxer::printBytes(AM_U8* p, AM_UINT size)
{
#ifdef AM_DEBUG
    if (!(mLogOption & LogBinaryData))
        return;

    if (!p)
        return;

    while (size > 3) {
        AMLOG_BINARY("  %2.2x %2.2x %2.2x %2.2x.\n", p[0], p[1], p[2], p[3]);
        p += 4;
        size -= 4;
    }

    if (size == 3) {
        AMLOG_BINARY("  %2.2x %2.2x %2.2x.\n", p[0], p[1], p[2]);
    } else if (size == 2) {
        AMLOG_BINARY("  %2.2x %2.2x.\n", p[0], p[1]);
    } else if (size == 1) {
        AMLOG_BINARY("  %2.2x.\n", p[0]);
    }
#endif
}

AM_INT CRTSPDemuxer::ParseMedia(struct parse_file_s *pParseFile, struct parser_obj_s *pParser)
{
    AM_ERROR("please implement.\n");
    return 1;
}

//is blow enough?
#define DMAX_VIDEO_BUFFER_SIZE 1024*256
#define DMAX_AUDIO_BUFFER_SIZE 1024*2*2

//-----------------------------------------------------------------------
//
// CRTSPDemuxer
//
//-----------------------------------------------------------------------
IFilter* CRTSPDemuxer::Create(IEngine *pEngine)
{
    CRTSPDemuxer *result = new CRTSPDemuxer(pEngine);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CRTSPDemuxer::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    DSetModuleLogConfig(LogModuleFFMpegDemuxer);

    // video output pin & bp
    if ((mpVideoOutput = CRTSPDemuxerOutput::Create(this)) == NULL)
        return ME_ERROR;

    if ((mpVideoBP = CFixedBufferPool::Create(DMAX_VIDEO_BUFFER_SIZE, 32)) == NULL)
        return ME_ERROR;

    mpVideoOutput->SetBufferPool(mpVideoBP);

    // audio output pin & bp
    if ((mpAudioOutput = CRTSPDemuxerOutput::Create(this)) == NULL)
        return ME_ERROR;

    if ((mpAudioBP = CFixedBufferPool::Create(DMAX_AUDIO_BUFFER_SIZE, 32)) == NULL)
        return ME_ERROR;

    mpAudioOutput->SetBufferPool(mpAudioBP);

    mVideoPipeFd[0] = -1;
    mVideoPipeFd[1] = -1;
    mAudioPipeFd[0] = -1;
    mAudioPipeFd[1] = -1;

    pipe(mVideoPipeFd);
    pipe(mAudioPipeFd);

    return ME_OK;
}

CRTSPDemuxer::~CRTSPDemuxer()
{
    AMLOG_DESTRUCTOR("~CRTSPDemuxer.\n");

    AM_RELEASE(mpVideoBP);
    AM_RELEASE(mpAudioBP);

    AM_DELETE(mpVideoOutput);
    AM_DELETE(mpAudioOutput);

    if (mVideoPipeFd[0] >= 0) {
        close(mVideoPipeFd[0]);
        mVideoPipeFd[0] = -1;
    }
    if (mVideoPipeFd[1] >= 0) {
        close(mVideoPipeFd[1]);
        mVideoPipeFd[1] = -1;
    }
    if (mAudioPipeFd[0] >= 0) {
        close(mAudioPipeFd[0]);
        mAudioPipeFd[0] = -1;
    }
    if (mAudioPipeFd[1] >= 0) {
        close(mAudioPipeFd[1]);
        mAudioPipeFd[1] = -1;
    }

    AMLOG_DESTRUCTOR("~CRTSPDemuxer done.\n");
}

void *CRTSPDemuxer::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IDemuxer)
        return (IDemuxer*)this;

    return inherited::GetInterface(refiid);
}

void CRTSPDemuxer::GetInfo(INFO& info)
{
    inherited::GetInfo(info);
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = 0;
    info.mIndex = 0;

    info.nOutput = mnOutputPin;
}

IPin* CRTSPDemuxer::GetOutputPin(AM_UINT index)
{
    if (index < mnOutputPin) {
        return mpOutput[index];
    }
    return NULL;
}

void CRTSPDemuxer::Delete()
{
    AMLOG_DESTRUCTOR("CRTSPDemuxer::Delete().\n");
    AM_RELEASE(mpVideoBP);
    mpVideoBP = NULL;

    AM_RELEASE(mpAudioBP);
    mpAudioBP = NULL;

    AM_DELETE(mpVideoOutput);
    mpVideoOutput = NULL;

    AM_DELETE(mpAudioOutput);
    mpAudioOutput = NULL;

    inherited::Delete();

    if (mVideoPipeFd[0] >= 0) {
        close(mVideoPipeFd[0]);
        mVideoPipeFd[0] = -1;
    }
    if (mVideoPipeFd[1] >= 0) {
        close(mVideoPipeFd[1]);
        mVideoPipeFd[1] = -1;
    }
    if (mAudioPipeFd[0] >= 0) {
        close(mAudioPipeFd[0]);
        mAudioPipeFd[0] = -1;
    }
    if (mAudioPipeFd[1] >= 0) {
        close(mAudioPipeFd[1]);
        mAudioPipeFd[1] = -1;
    }
}

bool CRTSPDemuxer::ProcessCmd(CMD& cmd, simple_queue_t* video_queue, simple_queue_t* audio_queue)
{
    AM_ASSERT(msState != STATE_HAS_OUTPUTBUFFER);
    AMLOG_CMD("****CRTSPDemuxer::ProcessCmd, cmd.code %d, state %d.\n", cmd.code, msState);
    char wake_char = 'w';

    switch (cmd.code) {
        case CMD_STOP:
            mbRun = false;
            CmdAck(ME_OK);
            break;

        case CMD_OBUFNOTIFY:
            if (cmd.pExtra == mpVideoBP) {
                //video
                AM_ASSERT(video_queue);
                if (video_queue) {
                    write(mVideoPipeFd[0], &wake_char, 1);
                    video_queue->enqueue(video_queue, CONTROL_CMD_BUFFER_NOTIFY);
                }
            } else if (cmd.pExtra == mpAudioBP) {
                //audio
                if (audio_queue) {
                    write(mAudioPipeFd[0], &wake_char, 1);
                    audio_queue->enqueue(audio_queue, CONTROL_CMD_BUFFER_NOTIFY);
                }
            } else {
                AM_ERROR("BAD cmd.\n");
            }
            break;

        case CMD_PAUSE:
            AM_ASSERT(!mbPaused);
            mbPaused = true;
            break;

        case CMD_RESUME:
            if(msState == STATE_PENDING)
                msState = STATE_IDLE;

            mbPaused = false;
            break;

        case CMD_FLUSH:
            AMLOG_INFO("CRTSPDemuxer: CMD_FLUSH STATE %d .\n",msState);
            msState = STATE_PENDING;
            CmdAck(ME_OK);
            AMLOG_INFO("CRTSPDemuxer: CMD_FLUSH done, STATE %d .\n",msState);
            break;

        case CMD_AVSYNC:
            CmdAck(ME_OK);
            break;

        case CMD_BEGIN_PLAYBACK:
            AMLOG_INFO("CRTSPDemuxer: CMD_BEGIN_PLAYBACK, STATE %d .\n",msState);
            AM_ASSERT(msState == STATE_PENDING);
            msState = STATE_IDLE;
            break;

        //only NVR rtsp streams can come here
        case CMD_RUN:
            AM_ASSERT(mpSharedRes->mDSPmode == 16);
            CmdAck(ME_OK);
            break;

        case CMD_SOURCE_FILTER_BLOCKED:
            break;

        default:
            AM_ERROR("wrong cmd %d.\n",cmd.code);
    }
    return false;
}

void CRTSPDemuxer::OnRun()
{
    CmdAck(ME_OK);

    CBuffer *pBuffer;
    CMD cmd;
    CRTSPDemuxerOutput* pOutput = NULL;
    AM_INT ret;
    mbRun = true;
    msState = STATE_IDLE;

    pthread_attr_t attr;
    struct sched_param param;

    pthread_t   video_thread;
    pthread_t   audio_thread;

    SRTPThreadParams video_params;
    SRTPThreadParams audio_params;
    struct sockaddr_in localhost_addr;
    
    AM_ASSERT(mpServerAddr);
    if (mpServerAddr) {
        //server
        memset(&mServerAddrIn, 0x0, sizeof(mServerAddrIn));
        mServerAddrIn.sin_family = AF_INET;
        mServerAddrIn.sin_port = mServerRTSPPort;
        mServerAddrIn.sin_addr.s_addr = inet_addr(mpServerAddr);

        //client, localhost
        memset(&localhost_addr, 0x0, sizeof(localhost_addr));
        localhost_addr.sin_family = AF_INET;
        localhost_addr.sin_port = 0;
        localhost_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        //start data recieve thread
        if (mpVideoBP && mbEnableVideo) {
            pthread_attr_init(&attr);
            pthread_attr_setschedpolicy(&attr, SCHED_RR);
            param.sched_priority = 80;  //todo, hardcode now
            pthread_attr_setschedparam(&attr, &param);

            video_params.bufferpool = mpVideoBP;
            video_params.format = IParameters::StreamFormat_H264;
            video_params.outpin = mpVideoOutput;
            video_params.pipe_fd = mVideoPipeFd[1];
            video_params.queue = _create_simple_queue(0);
            video_params.socket = mRTPVideoSocket;
            video_params.src_addr = *((struct sockaddr*)&mServerAddrIn);

            video_params.thiz = this;
            ret = pthread_create(&video_thread, &attr, recievingDataThread, (void*)(&video_params));
        }

        //start data recieve thread
        if (mpAudioBP && mbEnableAudio) {
            pthread_attr_init(&attr);
            pthread_attr_setschedpolicy(&attr, SCHED_RR);
            param.sched_priority = 80;  //todo, hardcode now
            pthread_attr_setschedparam(&attr, &param);

            audio_params.bufferpool = mpVideoBP;
            audio_params.format = IParameters::StreamFormat_AAC;
            audio_params.outpin = mpAudioOutput;
            audio_params.pipe_fd = mAudioPipeFd[1];
            audio_params.queue = _create_simple_queue(0);
            audio_params.socket = mRTPAudioSocket;
            audio_params.src_addr = *((struct sockaddr*)&mServerAddrIn);

            audio_params.thiz = this;
            ret = pthread_create(&video_thread, &attr, recievingDataThread, (void*)(&audio_params));
        }
    } else {
        AM_ERROR("please connect server first, then invoke Run.\n");
        msState = STATE_ERROR;
    }

    while (mbRun) {

#ifdef AM_DEBUG
        AMLOG_STATE("RTSPDemuxer: start switch, msState=%d.\n", msState);
        if (mpVideoBP && mbEnableVideo) {
            AMLOG_STATE("video buffers %d.\n", mpVideoBP->GetFreeBufferCnt());
        }
        if (mpAudioBP && mbEnableAudio) {
            AMLOG_STATE("audio buffers %d.\n", mpAudioBP->GetFreeBufferCnt());
        }
#endif

        switch (msState) {
            case STATE_IDLE:
                //wait cmd
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd, video_params.queue, audio_params.queue);
                break;

            case STATE_PENDING:
                //wait cmd
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd, video_params.queue, audio_params.queue);
                break;

            default:
                AM_ERROR("demuxer error state %d.\n", msState);
        }

    }
    AMLOG_INFO("CRTSPDemuxer: OnRun exit.\n");
}

bool CRTSPDemuxer::SendEOS(CRTSPDemuxerOutput *pPin)
{
    CBuffer *pBuffer;

    AMLOG_INFO("CRTSPDemuxer Send EOS begin.\n");

    if (!pPin->AllocBuffer(pBuffer))
        return false;

    pBuffer->SetType(CBuffer::EOS);
    pPin->SendBuffer(pBuffer);
    AMLOG_INFO("CRTSPDemuxer Send EOS end.\n");
    return true;
}

AM_ERR CRTSPDemuxer::LoadFile(const char *pFileName, void *context)
{
    const char* rtsp_prefix = "rtsp://";
    char* ptmp = NULL;
    AM_INT ret;

    AMLOG_INFO("[rtsp flow]: parse url start.\n");

    if (!pFileName) {
        AM_ERROR("NULL pFileName\n");
        return ME_BAD_PARAM;
    }

    if ((strlen(pFileName) < strlen(rtsp_prefix)) || strncmp(rtsp_prefix, pFileName, strlen(rtsp_prefix))) {
        AM_ERROR("BAD rtsp url: %s, should be like '%s10.0.0.2/stream_0'.\n", pFileName, rtsp_prefix);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(!mpServerAddr);
    if (mpServerAddr) {
        free(mpServerAddr);
    }
    mpServerAddr = (char*)malloc(strlen(pFileName));

    AM_ASSERT(!mpItemName);
    if (mpItemName) {
        free(mpItemName);
    }
    mpItemName = (char*)malloc(strlen(pFileName));

    if (mpServerAddr && mpItemName) {
        memset(mpServerAddr, 0x0, strlen(mpServerAddr));
        memset(mpItemName, 0x0, strlen(mpItemName));
    } else {
        AM_ERROR("malloc fail, please check code.\n");
        return ME_NO_MEMORY;
    }

    if (strchr(pFileName + strlen(rtsp_prefix), ':')) {
        //with port number
        ret = sscanf(pFileName, "rtsp://%s:%hu/%s", mpServerAddr, &mServerRTSPPort, mpItemName);
        if (3 != ret) {
            AM_ERROR("Parse rtsp url fail, %s, ret %d.\n", pFileName, ret);
            return ME_BAD_PARAM;
        }
    } else {
        //without prot number
        ret = sscanf(pFileName, "rtsp://%s/%s", mpServerAddr, mpItemName);
        if (2 != ret) {
            AM_ERROR("Parse rtsp url fail, %s, ret %d.\n", pFileName, ret);
            return ME_BAD_PARAM;
        }
    }

    AMLOG_INFO("[rtsp flow]: Parse rtsp url done, server addr %s, port %d, item name %s, start connect to server....\n", mpServerAddr, mServerRTSPPort, mpItemName);

    //connect to server, use tcp
    mRTSPSocket = AMNet_ConnectTo((const char *) mpServerAddr, mServerRTSPPort, SOCK_STREAM, IPPROTO_TCP);

    if (mRTSPSocket >= 0) {
        AMLOG_INFO("[rtsp flow]: connect to server done.\n");
    } else {
        AM_ERROR("[rtsp flow]: connect to server fail.\n");
        return ME_ERROR;
    }

    //send OPTIONS request
    memset(mRTSPBuffer, 0x0, DMAX_RTSP_STRING_LEN);
    snprintf(mRTSPBuffer, DMAX_RTSP_STRING_LEN - 1, _rtsp_option_request_fmt, mpServerAddr, mpItemName, mRTSPSeq ++);
    AMLOG_INFO("[rtsp flow]: before send OPTIONS request....\r\n    %s, length %d\n", mRTSPBuffer, strlen(mRTSPBuffer));
    ret = AMNet_Send(mRTSPSocket, (AM_U8*)mRTSPBuffer, strlen(mRTSPBuffer), 0);
    AMLOG_INFO("[rtsp flow]: send OPTIONS request done, sent len %d, read responce from server\n", ret);
    AM_ASSERT(ret == ((AM_INT)strlen(mRTSPBuffer)));

    ret = AMNet_Recv(mRTSPSocket, (AM_U8*)mRTSPBuffer, DMAX_RTSP_STRING_LEN, 0);
    AMLOG_INFO("[rtsp flow]: OPTIONS's responce from server:   %s, length %d\n", mRTSPBuffer, ret);
    //need parse responce here, fix me
    AMLOG_WARN("please implement parsing OPTIONS's responce, todo...\n");

    //send DESCRIBE request
    memset(mRTSPBuffer, 0x0, DMAX_RTSP_STRING_LEN);
    snprintf(mRTSPBuffer, DMAX_RTSP_STRING_LEN - 1, _rtsp_describe_request_fmt, mpServerAddr, mpItemName, mRTSPSeq ++);
    AMLOG_INFO("[rtsp flow]: before send DESCRIBE request....\r\n    %s, length %d\n", mRTSPBuffer, strlen(mRTSPBuffer));
    ret = AMNet_Send(mRTSPSocket, (AM_U8*)mRTSPBuffer, strlen(mRTSPBuffer), 0);
    AMLOG_INFO("[rtsp flow]: send DESCRIBE request done, sent len %d, read responce from server\n", ret);
    AM_ASSERT(ret == ((AM_INT)strlen(mRTSPBuffer)));

    ret = AMNet_Recv(mRTSPSocket, (AM_U8*)mRTSPBuffer, DMAX_RTSP_STRING_LEN, 0);
    AMLOG_INFO("[rtsp flow]: DESCRIBE's responce from server:   %s, length %d\n", mRTSPBuffer, ret);
    //need parse responce here, fix me
    AMLOG_WARN("please implement parsing DESCRIBE's responce, todo...\n");
    //hard code here, fix me
    mVideoHeight = 720;
    mVideoWidth = 1280;
    mVideoTrackID = 0;
    mAudioTrackID = 1;
    mVideoFormat = IParameters::StreamFormat_H264;
    mAudioFormat = IParameters::StreamFormat_AAC;
    mAudioChannelNumber = 2;
    mAudioSampleRate = 44100;

    //send SETUP request
    memset(mRTSPBuffer, 0x0, DMAX_RTSP_STRING_LEN);
    snprintf(mRTSPBuffer, DMAX_RTSP_STRING_LEN - 1, _rtsp_setup_request_fmt, mpServerAddr, mServerRTSPPort, mpItemName, mVideoTrackID, mRTSPSeq ++);
    AMLOG_INFO("[rtsp flow]: before send SETUP request....\r\n    %s, length %d\n", mRTSPBuffer, strlen(mRTSPBuffer));
    ret = AMNet_Send(mRTSPSocket, (AM_U8*)mRTSPBuffer, strlen(mRTSPBuffer), 0);
    AMLOG_INFO("[rtsp flow]: send SETUP request done, sent len %d, read responce from server\n", ret);
    AM_ASSERT(ret == ((AM_INT)strlen(mRTSPBuffer)));

    ret = AMNet_Recv(mRTSPSocket, (AM_U8*)mRTSPBuffer, DMAX_RTSP_STRING_LEN, 0);
    AMLOG_INFO("[rtsp flow]: SETUP's responce from server:   %s, length %d\n", mRTSPBuffer, ret);

    //find server's RTP/RTCP port(video)
    if ((ptmp = _find_string((char*)mRTSPBuffer, "server_port=", strlen("server_port=")))) {
        sscanf(ptmp, "server_port=%hu-%hu", &mServerRTPVideoPort, &mServerRTCPVideoPort);
        AMLOG_INFO("[rtsp flow]: in SETUP's responce, server's video RTP port %d, RTCP port %d.\n", mServerRTPVideoPort, mServerRTCPVideoPort);
    } else {
        AM_ERROR("cannot find server RTP/RTCP(video) port.\n");
        return ME_ERROR;
    }

    //find sessionID
    if ((ptmp = _find_string((char*)mRTSPBuffer, "Session: ", strlen("Session: ")))) {
        sscanf(ptmp, "Session: %llx", &mSessionID);
        AMLOG_INFO("[rtsp flow]: in SETUP's responce, get session ID %llx.\n", mSessionID);
    } else {
        AM_ERROR("cannot find session ID.\n");
        return ME_ERROR;
    }

    //send SETUP request
    memset(mRTSPBuffer, 0x0, DMAX_RTSP_STRING_LEN);
    snprintf(mRTSPBuffer, DMAX_RTSP_STRING_LEN - 1, _rtsp_setup_request_with_sessionid_fmt, mpServerAddr, mServerRTSPPort, mpItemName, mAudioTrackID, mRTSPSeq ++, mSessionID);
    AMLOG_INFO("[rtsp flow]: before send SETUP request....\r\n    %s, length %d\n", mRTSPBuffer, strlen(mRTSPBuffer));
    ret = AMNet_Send(mRTSPSocket, (AM_U8*)mRTSPBuffer, strlen(mRTSPBuffer), 0);
    AMLOG_INFO("[rtsp flow]: send SETUP request done, sent len %d, read responce from server\n", ret);
    AM_ASSERT(ret == ((AM_INT)strlen(mRTSPBuffer)));

    ret = AMNet_Recv(mRTSPSocket, (AM_U8*)mRTSPBuffer, DMAX_RTSP_STRING_LEN, 0);
    AMLOG_INFO("[rtsp flow]: SETUP's responce from server:   %s, length %d\n", mRTSPBuffer, ret);

    //find server's RTP/RTCP port(audio)
    if ((ptmp = _find_string((char*)mRTSPBuffer, "server_port=", strlen("server_port=")))) {
        sscanf(ptmp, "server_port=%hu-%hu", &mServerRTPAudioPort, &mServerRTCPAudioPort);
        AMLOG_INFO("[rtsp flow]: in SETUP's responce, server's audio RTP port %d, RTCP port %d.\n", mServerRTPAudioPort, mServerRTCPAudioPort);
    } else {
        AM_ERROR("cannot find server RTP/RTCP(audio) port.\n");
        return ME_ERROR;
    }

    //send PLAY request
    memset(mRTSPBuffer, 0x0, DMAX_RTSP_STRING_LEN);
    snprintf(mRTSPBuffer, DMAX_RTSP_STRING_LEN - 1, _rtsp_play_request_fmt, mpServerAddr, mServerRTSPPort, mpItemName, mRTSPSeq ++, mSessionID);
    AMLOG_INFO("[rtsp flow]: before send PLAY request....\r\n    %s, length %d\n", mRTSPBuffer, strlen(mRTSPBuffer));
    ret = AMNet_Send(mRTSPSocket, (AM_U8*)mRTSPBuffer, strlen(mRTSPBuffer), 0);
    AMLOG_INFO("[rtsp flow]: send PLAY request done, sent len %d, read responce from server\n", ret);
    AM_ASSERT(ret == ((AM_INT)strlen(mRTSPBuffer)));

    ret = AMNet_Recv(mRTSPSocket, (AM_U8*)mRTSPBuffer, DMAX_RTSP_STRING_LEN, 0);
    AMLOG_INFO("[rtsp flow]: PLAY's responce from server:   %s, length %d\n", mRTSPBuffer, ret);
    //need parse responce here, fix me
    AMLOG_WARN("please implement parsing PLAY's responce, todo...\n");

    return ME_OK;
}

void CRTSPDemuxer::sendTeardown()
{
    AM_INT ret = 0;
    //send PLAY request
    memset(mRTSPBuffer, 0x0, DMAX_RTSP_STRING_LEN);
    snprintf(mRTSPBuffer, DMAX_RTSP_STRING_LEN - 1, _rtsp_teardown_request_fmt, mpServerAddr, mServerRTSPPort, mpItemName, mRTSPSeq ++, mSessionID);
    AMLOG_INFO("[rtsp flow]: before send TEARDWON request....\r\n    %s, length %d\n", mRTSPBuffer, strlen(mRTSPBuffer));
    ret = AMNet_Send(mRTSPSocket, (AM_U8*)mRTSPBuffer, strlen(mRTSPBuffer), 0);
    AMLOG_INFO("[rtsp flow]: send TEARDWON request done, sent len %d, read responce from server\n", ret);
    AM_ASSERT(ret == ((AM_INT)strlen(mRTSPBuffer)));

    ret = AMNet_Recv(mRTSPSocket, (AM_U8*)mRTSPBuffer, DMAX_RTSP_STRING_LEN, 0);
    AMLOG_INFO("[rtsp flow]: TEARDWON's responce from server:   %s, length %d\n", mRTSPBuffer, ret);
    //need parse responce here, fix me
    AMLOG_WARN("please implement parsing TEARDWON's responce, todo...\n");
}

void* CRTSPDemuxer::recievingDataThread(void* _param)
{
    SRTPThreadParams* params = (SRTPThreadParams*)_param;
    AM_INT mMaxFd;
    CRTSPDemuxer* thiz;
    CRTSPDemuxerOutput* outpin;
    CFixedBufferPool* p_buffer_pool;
    AM_INT socket;
    struct sockaddr src_addr;
    AM_INT pipe_fd;
    AM_UINT state;
    AM_INT nready = 0;
    simple_queue_t* queue = params->queue;
    AM_UINT cmd;
    AM_UINT run = 1;

    AM_INT total_write_len = 0;
    AM_U8* p_cur = NULL;
    AM_INT read_len;

    AM_INT last_rtp_packet = 0;

    AM_U8 reserved[32];
    AM_INT rev_data_len;
    AM_INT start_code_len;
    AM_INT rtp_header_len;

    AM_UINT all_in_one_rtp_packet = 0;

    CBuffer* pBuffer = NULL;
    char char_buffer;

    AM_ASSERT(params);
    if (!params || !queue) {
        return NULL;
    }

    thiz = params->thiz;
    outpin = params->outpin;
    socket = params->socket;
    src_addr = params->src_addr;
    pipe_fd = params->pipe_fd;
    p_buffer_pool = params->bufferpool;

    AM_ASSERT(outpin);
    AM_ASSERT(socket >= 0);
    AM_ASSERT(pipe_fd >= 0);
    AM_ASSERT(p_buffer_pool);

    fd_set mAllSet;
    fd_set mReadSet;

    //init
    FD_ZERO(&mAllSet);
    FD_SET(pipe_fd, &mAllSet);
    FD_SET(socket, &mAllSet);
    if (socket > pipe_fd) {
        mMaxFd = socket;
    } else {
        AM_ASSERT(pipe_fd != socket);
        mMaxFd = pipe_fd;
    }

    if (IParameters::StreamFormat_H264 == params->format) {
        //h264 related
        start_code_len = 4;
        rtp_header_len = 14;
        rev_data_len = rtp_header_len;
    } else if (IParameters::StreamFormat_AAC == params->format) {
        //aac related
        start_code_len = 0;
        rtp_header_len = 16;
        rev_data_len = rtp_header_len - start_code_len;
    } else {
        AM_ERROR("please add RTP parser support for %d.\n", params->format);
        return NULL;
    }
    AM_ASSERT(rev_data_len < 32);

    //init state
    if (p_buffer_pool->GetFreeBufferCnt()) {
        if (!outpin->AllocBuffer(pBuffer)) {
            AM_ERROR("demuxer AllocBuffer Fail, must not comes here!\n");
            run = 0;
        }
        state = DATA_THREAD_STATE_READ_FIRST_RTP_PACKET;
    } else {
        AM_ERROR("why no free buffer at start?\n");
        FD_CLR(socket, &mAllSet);
        state = DATA_THREAD_STATE_WAIT_OUTBUFFER;
    }

    while (run) {

        switch (state) {

            case DATA_THREAD_STATE_READ_FIRST_RTP_PACKET:
                AM_ASSERT(pBuffer);

                nready = select(mMaxFd+1, &mReadSet, NULL, NULL, NULL);
                AM_DBG("[Data thread]: after select.\n");

                //process cmd
                if (FD_ISSET(pipe_fd, &mReadSet)) {
                    //some input from engine, process cmd first
                    AM_DBG("[Data thread]: from pipe fd.\n");
                    nready --;
                    while (queue->peekqueue(queue, &cmd)) {
                        read(pipe_fd, &char_buffer, sizeof(char_buffer));
                        if (CONTROL_CMD_QUIT == cmd) {
                            run = 0;
                            break;
                        } else if (CONTROL_CMD_BUFFER_NOTIFY == cmd) {
                            //skip in this case
                            break;
                        } else if (CONTROL_CMD_SPEEDUP == cmd) {
                            //goto skip data state
                            state = DATA_THREAD_STATE_SKIP_DATA;
                            break;
                        } else {
                            AM_ERROR("Unknown cmd %d.\n", cmd);
                            break;
                        }
                    }
                    break;
                }

                if (FD_ISSET(socket, &mReadSet)) {
                    if (!total_write_len) {
                        //write from start
                        p_cur = pBuffer->mpDataBase;
                    } else {
                        AM_ERROR("total_write_len %d, p_cur %p should be zero, at the beginning.\n", total_write_len, p_cur);
                        total_write_len = 0;
                        p_cur = pBuffer->mpDataBase;
                    }

                    read_len = AMNet_RecvFrom(socket, p_cur, pBuffer->mBlockSize - total_write_len, 0, (const struct sockaddr *)&src_addr, sizeof(sockaddr));
                    AM_ASSERT(read_len <= (((AM_INT)pBuffer->mBlockSize) - total_write_len));

                    //all in one rtp packet, first packet has marker bit
                    AM_ASSERT(!total_write_len);
                    if ((!total_write_len) && (p_cur[1] & 0x80)) {
                        //aac audio
                        if (IParameters::StreamFormat_AAC == params->format) {
                            pBuffer->SetDataPtr(pBuffer->mpDataBase + rev_data_len);
                            pBuffer->SetType(CBuffer::DATA);
                            pBuffer->SetDataSize((AM_UINT)(read_len - rev_data_len));

                            //assert the data size is correct

                            //send packet
                            outpin->SendBuffer(pBuffer);
                            pBuffer = NULL;

                            if (p_buffer_pool->GetFreeBufferCnt()) {
                                if (!outpin->AllocBuffer(pBuffer)) {
                                    AM_ERROR("demuxer AllocBuffer Fail, must not comes here!\n");
                                    run = 0;
                                    state = DATA_THREAD_STATE_ERROR;
                                    break;
                                }
                            } else {
                                FD_CLR(socket, &mAllSet);
                                state = DATA_THREAD_STATE_WAIT_OUTBUFFER;
                                pBuffer = NULL;
                            }

                        }else if (IParameters::StreamFormat_H264 == params->format) {
                            //fill start code prefix
                            p_cur += rev_data_len - 2 - start_code_len;//no nal type and nri byte
                            p_cur[0] = 0;
                            p_cur[1] = 0;
                            p_cur[2] = 0;
                            p_cur[3] = 0x1; //start code prefix

                            pBuffer->SetDataPtr(p_cur);
                            pBuffer->SetType(CBuffer::DATA);
                            pBuffer->SetDataSize((AM_UINT)(read_len + start_code_len + 2 - rev_data_len));

                            //send packet
                            outpin->SendBuffer(pBuffer);
                            pBuffer = NULL;

                            if (p_buffer_pool->GetFreeBufferCnt()) {
                                if (!outpin->AllocBuffer(pBuffer)) {
                                    AM_ERROR("demuxer AllocBuffer Fail, must not comes here!\n");
                                    run = 0;
                                    state = DATA_THREAD_STATE_ERROR;
                                    break;
                                }
                            } else {
                                FD_CLR(socket, &mAllSet);
                                pBuffer = NULL;
                                state = DATA_THREAD_STATE_WAIT_OUTBUFFER;
                            }
                        }
                        //clear
                        total_write_len = 0;
                        break;
                    }

                    //blow is more than one rtp packet, need re-packet it to a total es packet, code is tricky to avoid memory copy
                    //please be caution when modify it

                    //fill start code prefix with first packet
                    p_cur[rev_data_len] = 0;
                    p_cur[rev_data_len + 1] = 0;
                    p_cur[rev_data_len + 2] = 0;
                    p_cur[rev_data_len + 3] = 0x1;

                    //update write pointer
                    p_cur += read_len;

                    //debug assert
                    AM_ASSERT(IParameters::StreamFormat_H264 == params->format);

                    //reserve data
                    p_cur -= rev_data_len;
                    memcpy(reserved, p_cur, rev_data_len);
                    total_write_len += read_len - rev_data_len;
                }
                break;

            case DATA_THREAD_STATE_READ_REMANING_RTP_PACKET:
                AM_ASSERT(pBuffer);
                //only h264 comes into this case, aac packet should within a rtp packet
                AM_ASSERT(IParameters::StreamFormat_AAC == params->format);

                nready = select(mMaxFd+1, &mReadSet, NULL, NULL, NULL);
                AM_DBG("[Data thread]: after select.\n");

                //process cmd
                if (FD_ISSET(pipe_fd, &mReadSet)) {
                    //some input from engine, process cmd first
                    AM_DBG("[Data thread]: from pipe fd.\n");
                    nready --;
                    while (queue->peekqueue(queue, &cmd)) {
                        read(pipe_fd, &char_buffer, sizeof(char_buffer));
                        if (CONTROL_CMD_QUIT == cmd) {
                            run = 0;
                            break;
                        } else if (CONTROL_CMD_BUFFER_NOTIFY == cmd) {
                            //skip in this case
                            break;
                        } else if (CONTROL_CMD_SPEEDUP == cmd) {
                            //goto skip data state
                            state = DATA_THREAD_STATE_SKIP_DATA;
                            break;
                        } else {
                            AM_ERROR("Unknown cmd %d.\n", cmd);
                            break;
                        }
                    }
                    break;
                }

                if (FD_ISSET(socket, &mReadSet)) {

                    //debug assert
                    AM_ASSERT(total_write_len);
                    AM_ASSERT(pBuffer);
                    AM_ASSERT(p_cur != pBuffer->mpDataBase);

                    read_len = AMNet_RecvFrom(socket, p_cur, pBuffer->mBlockSize - total_write_len - rev_data_len, 0, (const struct sockaddr *)&src_addr, sizeof(sockaddr));
                    AM_ASSERT(read_len <= (((AM_INT)pBuffer->mBlockSize) - total_write_len - rev_data_len));

                    //blow is more than one rtp packet, need re-packet it to a total es packet, code is tricky to avoid memory copy
                    //please be caution when modify it
                    if (p_cur[1] & 0x80) {
                        //last rtp packet, has marker bit
                        if (IParameters::StreamFormat_H264 == params->format) {
                            AM_ASSERT(total_write_len);
                            if (total_write_len) {
                                //restore previous reserved data
                                memcpy(p_cur, reserved, rev_data_len);
                            }

                            total_write_len += read_len - rev_data_len;

                            pBuffer->SetDataPtr(pBuffer->mpDataBase + rev_data_len);
                            pBuffer->SetType(CBuffer::DATA);
                            pBuffer->SetDataSize((AM_UINT)(total_write_len));

                            //send packet
                            outpin->SendBuffer(pBuffer);
                            pBuffer = NULL;

                            if (p_buffer_pool->GetFreeBufferCnt()) {
                                if (!outpin->AllocBuffer(pBuffer)) {
                                    AM_ERROR("demuxer AllocBuffer Fail, must not comes here!\n");
                                    run = 0;
                                    state = DATA_THREAD_STATE_ERROR;
                                    break;
                                }
                                state = DATA_THREAD_STATE_READ_FIRST_RTP_PACKET;
                            } else {
                                FD_CLR(socket, &mAllSet);
                                pBuffer = NULL;
                                state = DATA_THREAD_STATE_WAIT_OUTBUFFER;
                            }
                            //clear
                            total_write_len = 0;
                        }
                        break;
                    } else {
                        //not last packet
                        AM_ASSERT(total_write_len);
                        if (total_write_len) {
                            //restore previous reserved data
                            memcpy(p_cur, reserved, rev_data_len);
                        }

                        //update write pointer
                        p_cur += read_len;

                        //reserve data
                        p_cur -= rev_data_len;
                        memcpy(reserved, p_cur, rev_data_len);
                        total_write_len += read_len - rev_data_len;
                    }

                }else {
                    AM_ERROR("why comes here.\n");
                }
                break;

            case DATA_THREAD_STATE_WAIT_OUTBUFFER:
                AM_ASSERT(!pBuffer);

                if (p_buffer_pool->GetFreeBufferCnt()) {
                    if (!outpin->AllocBuffer(pBuffer)) {
                        AM_ERROR("demuxer AllocBuffer Fail, must not comes here!\n");
                        run = 0;
                    }
                    FD_SET(socket, &mAllSet);
                    state = DATA_THREAD_STATE_READ_FIRST_RTP_PACKET;
                    break;
                }

                nready = select(mMaxFd+1, &mReadSet, NULL, NULL, NULL);
                AM_DBG("[Data thread]: after select.\n");
                AM_ASSERT(1 == nready);

                //process cmd
                if (FD_ISSET(pipe_fd, &mReadSet)) {
                    AM_DBG("[Data thread]: from pipe fd.\n");
                    //some input from engine, process cmd first
                    while (queue->peekqueue(queue, &cmd)) {
                        read(pipe_fd, &char_buffer, sizeof(char_buffer));
                        if (CONTROL_CMD_QUIT == cmd) {
                            run = 0;
                            break;
                        } else if (CONTROL_CMD_BUFFER_NOTIFY == cmd) {
                            if (p_buffer_pool->GetFreeBufferCnt()) {
                                if (!outpin->AllocBuffer(pBuffer)) {
                                    AM_ERROR("demuxer AllocBuffer Fail, must not comes here!\n");
                                    run = 0;
                                    break;
                                }
                                FD_SET(socket, &mAllSet);
                                state = DATA_THREAD_STATE_READ_FIRST_RTP_PACKET;
                            } else {
                                //do nothing, continue wait
                                AM_INFO("still no buffer, continue wait.\n");
                            }
                            break;
                        } else if (CONTROL_CMD_SPEEDUP == cmd) {
                            //goto skip data state
                            state = DATA_THREAD_STATE_SKIP_DATA;
                            break;
                        } else {
                            AM_ERROR("Unknown cmd %d.\n", cmd);
                            break;
                        }
                    }
                } else {
                    AM_ERROR("why has another fd?\n");
                }
                break;

            case DATA_THREAD_STATE_SKIP_DATA:
                // to do, skip till next IDR
                AM_ERROR("add implenment.\n");
                break;

            case DATA_THREAD_STATE_ERROR:
                // to do, error case
                AM_ERROR("add implenment.\n");
                break;

            default:
                AM_ERROR("unexpected state %d\n", state);
                break;
        }
    }

    return NULL;
}

bool CRTSPDemuxer::isSupportedByDuplex()
{
    return true;
}

void CRTSPDemuxer::GetVideoSize(AM_INT *pWidth, AM_INT *pHeight) {
    AM_ASSERT(mVideoWidth && mVideoHeight);
    *pWidth = mVideoWidth;
    *pHeight = mVideoHeight;
}



AM_ERR CRTSPDemuxer::Seek(AM_U64 &ms)
{
    return ME_OK;
}

AM_ERR CRTSPDemuxer::GetTotalLength(AM_U64& ms)
{
    ms = 0;
    return ME_OK;
}

AM_ERR CRTSPDemuxer::GetFileType(const char *&pFileType)
{
    pFileType = "rtsp";
    return ME_OK;
}

void CRTSPDemuxer::EnableAudio(bool enable) {
    mbEnableAudio = enable;
    mpSharedRes->audioEnabled = enable;
}

void CRTSPDemuxer::EnableVideo(bool enable) {
    mbEnableVideo = enable;
    mpSharedRes->videoEnabled = enable;
}

#ifdef AM_DEBUG
void CRTSPDemuxer::PrintState()
{
    if (mbEnableVideo && mpVideoBP) {
        AM_ASSERT(mpVideoBP);
        AMLOG_INFO("FFDemuxer(video packet): msState=%d, %d free video buffers.\n", msState, mpVideoBP->GetFreeBufferCnt());
    }
    if (mbEnableAudio && mpAudioBP) {
        AM_ASSERT(mpAudioBP);
        AMLOG_INFO("FFDemuxer(audio packet): msState=%d, %d free audio buffers.\n", msState, mpAudioBP->GetFreeBufferCnt());
    }
}
#endif

//-----------------------------------------------------------------------
//
// CRTSPDemuxerOutput
//
//-----------------------------------------------------------------------
CRTSPDemuxerOutput* CRTSPDemuxerOutput::Create(CFilter *pFilter)
{
    CRTSPDemuxerOutput *result = new CRTSPDemuxerOutput(pFilter);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

CRTSPDemuxerOutput::CRTSPDemuxerOutput(CFilter *pFilter):
    inherited(pFilter)
{
    mpSharedRes = ((CActiveFilter*)pFilter)->mpSharedRes;
}

AM_ERR CRTSPDemuxerOutput::Construct()
{
    DSetModuleLogConfig(LogModuleFFMpegDemuxer);
	return ME_OK;
}

CRTSPDemuxerOutput::~CRTSPDemuxerOutput()
{
    AMLOG_DESTRUCTOR("~CRTSPDemuxerOutput.\n");
}

