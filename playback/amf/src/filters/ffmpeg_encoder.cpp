/*
 * ffmpeg_encoder.cpp
 *
 * History:
 *    2011/7/27 - [Jay Zhang] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#define LOG_NDEBUG 0
#define LOG_TAG "ffmpeg_encoder"
#define AMDROID_DEBUG

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "record_if.h"
#include "engine_guids.h"
#include "am_record_if.h"
#include "am_util.h"
extern "C" {
#define INT64_C(a) (a ## LL)
#include "aac_audio_enc.h"
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}
#include "ffmpeg_encoder.h"

#define DMAX_FADE_CNT 8
#define DDATA_CHUNK_CNT 4
static AM_UINT fade_level[DMAX_FADE_CNT] = {6,5,4,3,2,2,1,1};

static void _fade_audio_input_short(AM_S16* pdata, AM_UINT data_size, AM_UINT shift)
{
    while (data_size) {
        data_size --;
        *pdata >>= shift;
        pdata ++;
    }
}


IFilter* CreateFFmpegAudioEncoder(IEngine * pEngine)
{
    return CFFMpegEncoder::Create(pEngine);
}

IFilter* CFFMpegEncoder::Create(IEngine * pEngine)
{
    CFFMpegEncoder * result = new CFFMpegEncoder(pEngine);

    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CFFMpegEncoder::Construct()
{
    AM_ERR err = inherited::Construct();
    DSetModuleLogConfig(LogModuleFFMpegEncoder);
    if (err != ME_OK) {
        AM_ERROR("inherited::Construct() fail in CFFMpegEncoder::Construct().\n");
        return err;
    }

    if ((mpInput = CFFMpegEncoderInput::Create(this)) == NULL) {
        AM_ERROR("!!!not enough memory? CFFMpegEncoderInput::Create(this) fail in CFFMpegEncoder::Construct().\n");
        return ME_NO_MEMORY;
    }

    if ((mpBufferPool = CSimpleBufferPool::Create("FFMpegEncoderBuffer", 64, sizeof(CBuffer) + MAX_AUDIO_BUFFER_SIZE)) == NULL) {
        AM_ERROR("!!!not enough memory? CSimpleBufferPool::Create fail in CFFMpegEncoder::Construct().\n");
        return ME_NO_MEMORY;
    }

    return ME_OK;
}

CFFMpegEncoder::~CFFMpegEncoder()
{
    AMLOG_DESTRUCTOR("~CFFMpegEncoder start, mpAudioEncoder %p.\n", mpAudioEncoder);
    if (mpAudioEncoder != NULL) {
        avcodec_close(mpAudioEncoder);
        av_free(mpAudioEncoder);
        mpAudioEncoder = NULL;
    }
    AMLOG_DESTRUCTOR("~CFFMpegEncoder, avcodec_close(mpAudioEncoder) done, mpInput %p.\n", mpInput);

    AM_DELETE(mpInput);
    mpInput = NULL;
    AMLOG_DESTRUCTOR("~CFFMpegEncoder, AM_DELETE(mpInput) done.\n");

    for (AM_INT i = 0; i < MAX_NUM_OUTPUT_PIN; i++) {
        AMLOG_DESTRUCTOR("~CFFMpegEncoder, AM_DELETE(mpOutput[%d]) start, mpOutput[%d] = %p.\n", i, i, mpOutput[i]);
        AM_DELETE(mpOutput[i]);
        mpOutput[i] = NULL;
        AMLOG_DESTRUCTOR("~CFFMpegEncoder, AM_DELETE(mpOutput[%d]) done.\n", i);
    }

    AMLOG_DESTRUCTOR("~CFFMpegEncoder, AM_RELEASE(mpBufferPool %p) start.\n", mpBufferPool);
    AM_RELEASE(mpBufferPool);
    mpBufferPool = NULL;
    AMLOG_DESTRUCTOR("~CFFMpegEncoder, AM_RELEASE(mpBufferPool) done.\n");

}

void CFFMpegEncoder::GetInfo(INFO& info)
{
	info.nInput = 1;
	info.nOutput = 1;
	info.pName = "FFMpegAudEnc";
}

IPin* CFFMpegEncoder::GetInputPin(AM_UINT index)
{
	if (index == 0) return mpInput;
	return NULL;
}

IPin* CFFMpegEncoder::GetOutputPin(AM_UINT index)
{
    printf("CFFMpegEncoder::GetOutputPin %d, %p\n", index, mpOutput[index]);
    if (index < MAX_NUM_OUTPUT_PIN) {
        return (IPin*)mpOutput[index];
    }
    return NULL;
}

AM_ERR CFFMpegEncoder::Stop()
{
    AMLOG_INFO("=== CFFMpegEncoder::Stop()\n");
    inherited::Stop();
    return ME_OK;
}

AM_ERR CFFMpegEncoder::AddOutputPin(AM_UINT& index, AM_UINT type)
{
    AM_UINT i;

    AM_ASSERT(type == IParameters::StreamType_Audio);
    if (type != IParameters::StreamType_Audio) {
        AM_ERROR("CFFMpegEncoder::AddOutputPin only support audio.\n");
        return ME_ERROR;
    }

    for (i = 0; i < MAX_NUM_OUTPUT_PIN; i++) {
        if (mpOutput[i] == NULL)
            break;
    }

    if (i >= MAX_NUM_OUTPUT_PIN) {
        AM_ERROR(" max output pin %d (max %d) reached, please check code.\n", i, MAX_NUM_OUTPUT_PIN);
        return ME_ERROR;
    }

    if ((mpOutput[i] = CFFMpegEncoderOutput::Create(this)) == NULL)
        return ME_NO_MEMORY;

    AM_ASSERT(mpBufferPool);
    mpOutput[i]->SetBufferPool(mpBufferPool);

    index = i;
    return ME_OK;
}

AM_ERR CFFMpegEncoder::SetParameters(StreamType type, StreamFormat format, UFormatSpecific* param, AM_UINT index)
{
    AM_ASSERT(type == StreamType_Audio);
    mAudioFormat = format;
    mSampleFmt = param->audio.sample_format;
    mSampleRate = param->audio.sample_rate;
    mNumOfChannels = param->audio.channel_number;
    mBitrate = param->audio.bitrate;
    mSpecifiedFrameSize = param->audio.frame_size;

    switch (mAudioFormat) {
        case StreamFormat_AAC:
            mCodecId = CODEC_ID_AAC;
            break;

        case StreamFormat_MP2:
            mCodecId = CODEC_ID_MP2;
            break;

        case StreamFormat_AC3:
            mCodecId = CODEC_ID_AC3;
            break;

        case StreamFormat_ADPCM:
            mCodecId = CODEC_ID_ADPCM_IMA_QT;
            break;

        case StreamFormat_AMR_WB:
            mCodecId = CODEC_ID_AMR_WB;
            break;

        case StreamFormat_AMR_NB:
            mCodecId = CODEC_ID_AMR_NB;
            break;

        default:
            AM_ASSERT(0);
            break;
    }

    AMLOG_INFO("CFFMpegEncoder: channel %d, samplerate %d, audioFmt %d, mBitrate %d, mSpecifiedFrameSize %d.\n", mNumOfChannels, mSampleRate, mAudioFormat, mBitrate, mSpecifiedFrameSize);
    return ME_OK;
}

AM_ERR CFFMpegEncoder::OpenEncoder()
{
    AM_ASSERT(NULL == mpAudioEncoder);
    if (mpAudioEncoder) {
        AMLOG_WARN("CFFMpegEncoder::OpenEncoder already opened, %p.\n", mpAudioEncoder);
        return ME_OK;
    }

    AVCodec *codec;
    avcodec_init();
    avcodec_register_all();
    AM_INT ret = 0;

    codec = avcodec_find_encoder(mCodecId);
    if (!codec) {
    	AM_ERROR("Failed to find encoder:%x\n",mCodecId);
    	return ME_ERROR;
    }
    mpAudioEncoder= avcodec_alloc_context();
    mpAudioEncoder->bit_rate = mBitrate;
    mpAudioEncoder->sample_rate = mSampleRate;
    mpAudioEncoder->channels = mNumOfChannels;
    mpAudioEncoder->codec_id = mCodecId;
    mpAudioEncoder->codec_type = codec->type;
    mpAudioEncoder->sample_fmt = (AVSampleFormat)mSampleFmt;

    if (1 == mpAudioEncoder->channels) {
        mpAudioEncoder->channel_layout = CH_LAYOUT_MONO;
    } else if (2 == mpAudioEncoder->channels) {
        mpAudioEncoder->channel_layout = CH_LAYOUT_STEREO;
    } else {
        AM_ERROR("channel >2 is not supported yet.\n");
        mpAudioEncoder->channels = 2;
        mpAudioEncoder->channel_layout = CH_LAYOUT_STEREO;
    }

    AMLOG_PRINTF(" audio parameters: bit rate %d, samplerate %d, channels %d.\n", mpAudioEncoder->bit_rate, mpAudioEncoder->sample_rate, mpAudioEncoder->channels);
    AMLOG_PRINTF("                          codec_id 0x%x, codec_type %d, sample_fmt %d.\n", mpAudioEncoder->codec_id, mpAudioEncoder->codec_type, mpAudioEncoder->sample_fmt);

    if ((ret = avcodec_open(mpAudioEncoder, codec)) < 0) {
        AM_ERROR("Failed to open codec, ret %d\n", ret);
        return ME_ERROR;
    }
    AMLOG_INFO("mpAudioEncoder->frame_size %d.\n", mpAudioEncoder->frame_size);
    return ME_OK;
}

void CFFMpegEncoder::SendoutBuffer(CBuffer* pBuffer)
{
    AM_ASSERT(pBuffer);
    if (NULL == pBuffer) {
        AM_ERROR("NULL pointer in CFFMpegEncoder::SendoutBuffer.\n");
        return;
    }

    //send out buffer
    for (AM_UINT i = 0; i < MAX_NUM_OUTPUT_PIN; i++) {
        if (mpOutput[i] == NULL)
            continue;
        pBuffer->AddRef();
        mpOutput[i]->SendBuffer(pBuffer);
    }
}

bool CFFMpegEncoder::ProcessCmd(CMD& cmd)
{
    AMLOG_CMD("****CFFMpegEncoder::ProcessCmd, cmd.code %d, state %d.\n", cmd.code, msState);
    switch (cmd.code) {

        case CMD_STOP:
            mbRun = false;
            CmdAck(ME_OK);
            AMLOG_INFO("****CFFMpegEncoder::ProcessCmd, STOP cmd.\n");
            break;

        case CMD_RUN:
            //re-run
            AMLOG_INFO("CFFMpegEncoder re-Run, state %d.\n", msState);
            AM_ASSERT(STATE_PENDING == msState || STATE_ERROR == msState);
            msState = STATE_IDLE;
            CmdAck(ME_OK);
            break;

        case CMD_OBUFNOTIFY:
            if (mpBufferPool->GetFreeBufferCnt() > 0) {
                if (msState == STATE_IDLE)
                    msState = STATE_HAS_OUTPUTBUFFER;
                else if(msState == STATE_HAS_INPUTDATA)
                    msState = STATE_READY;
            }
            break;

        default:
            AM_ERROR(" CFFMpegEncoder: wrong cmd %d.\n",cmd.code);
    }
    return false;
}

void CFFMpegEncoder::OnRun()
{
    CmdAck(ME_OK);
    CBuffer *pBuffer = NULL;
    CBuffer *pOutBuffer = NULL;
    AM_UINT encodeOutSize =0;
    AM_U8* outData = NULL;
    short *inData = NULL;
//    CFFMpegEncoderOutput *pCurrOutPin;
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
    char Dumpfilename[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN] = {0};


    if (OpenEncoder() != ME_OK) {
        AM_ERROR("ERROR!!!!,CFFMpegEncoder open encoder fail.\n");
        msState = STATE_ERROR;
    } else {
        msState = STATE_IDLE;
    }

    AM_ASSERT(mpBufferPool);
    AM_ASSERT(mpInput);
    if (NULL == mpBufferPool || NULL == mpInput) {
        AM_ERROR("NULL pointer: mpBufferPool(%p), mpInput(%p), must have errors.\n", mpBufferPool, mpInput);
        msState = STATE_ERROR;
    }

    //calculate some params, 1 second fade in
    if (mSampleRate && mSpecifiedFrameSize) {
        mFadeInMaxBufferNumber = (1*mSampleRate)/mSpecifiedFrameSize/DMAX_FADE_CNT;
        AMLOG_INFO("[fade in calculate]: mFadeInMaxBufferNumber %d, mSampleRate %d, mSpecifiedFrameSize %d.\n", mFadeInMaxBufferNumber, mSampleRate, mSpecifiedFrameSize);
    }

    mbRun = true;
    while (mbRun) {

        AMLOG_STATE("CFFMpegEncoder::OnRun loop begin, buffer cnt.\n");

#ifdef AM_DEBUG
        if (mpInput) {
            AMLOG_STATE(" inputpin have free data cnt %d.\n", mpInput->mpBufferQ->GetDataCnt());
        }
        if (mpBufferPool) {
            AMLOG_STATE(" buffer pool have %d free buffers.\n", mpBufferPool->GetFreeBufferCnt());
        }
#endif

        switch (msState) {
            case STATE_IDLE:
                AM_ASSERT(!pBuffer);
                AM_ASSERT(!pOutBuffer);
                if (mpBufferPool->GetFreeBufferCnt() > 0) {
                    msState = STATE_HAS_OUTPUTBUFFER;
                } else {
                    type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
                    if (type == CQueue::Q_MSG) {
                        ProcessCmd(cmd);
                    } else {
                        AM_ASSERT((CQueueInputPin*)result.pOwner == (CQueueInputPin*)mpInput);
                        if (!mpInput->PeekBuffer(pBuffer)) {
                            AM_ERROR("No buffer?\n");
                            msState = STATE_ERROR;
                            break;
                        }
                        msState = STATE_HAS_INPUTDATA;
                    }
                }
                break;

            case STATE_HAS_INPUTDATA:
                AM_ASSERT(pBuffer);
                AM_ASSERT(!pOutBuffer);

                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case STATE_HAS_OUTPUTBUFFER:
                AM_ASSERT(mpBufferPool->GetFreeBufferCnt() > 0);
                if (!pOutBuffer) {
                    //get outbuffer here
                    if (!mpBufferPool->AllocBuffer(pOutBuffer, 0)) {
                        AM_ERROR("Failed to AllocBuffer\n");
                        msState = STATE_ERROR;
                        break;
                    }
                }

                AM_ASSERT(!pBuffer);
                AM_ASSERT(pOutBuffer);

                type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
                if (type == CQueue::Q_MSG) {
                    ProcessCmd(cmd);
                } else {
                    AM_ASSERT((CQueueInputPin*)result.pOwner == (CQueueInputPin*)mpInput);
                    if (!mpInput->PeekBuffer(pBuffer)) {
                        AM_ERROR("No buffer?\n");
                        pOutBuffer->Release();
                        pOutBuffer = NULL;
                        msState = STATE_ERROR;
                        break;
                    }
                    msState = STATE_READY;
                }
                break;

            case STATE_READY:
                AM_ASSERT(pBuffer);
                if (!pOutBuffer) {
                    //get outbuffer here
                    if (!mpBufferPool->AllocBuffer(pOutBuffer, 0)) {
                        AM_ERROR("Failed to AllocBuffer\n");
                        msState = STATE_ERROR;
                        break;
                    }
                }
                AM_ASSERT(pOutBuffer);

                if (NULL == pBuffer || NULL == pOutBuffer) {
                    AM_ERROR("NULL buffer pointer: pBuffer(%p), pOutBuffer(%p).\n", pBuffer, pOutBuffer);
                    msState = STATE_ERROR;
                    break;
                }

                if (pBuffer->GetType() == CBuffer::DATA) {
                    outData = (AM_U8*)((AM_U8*)pOutBuffer+sizeof(CBuffer));
                    inData = (short *)pBuffer->GetDataPtr();

#ifdef AM_DEBUG
                    mDumpFileIndex++;
                    //dump input data
                    if (mLogOutput & LogDumpSeparateBinary) {
                        snprintf(Dumpfilename, sizeof(Dumpfilename), "%s/audio_raw", AM_GetPath(AM_PathDump));
                        AM_DumpBinaryFile_withIndex(Dumpfilename, mDumpFileIndex - 1, pBuffer->GetDataPtr(), pBuffer->GetDataSize());
                    }
                    if (mLogOutput & LogDumpTotalBinary) {
                        snprintf(Dumpfilename, sizeof(Dumpfilename), "%s/audio_raw.total", AM_GetPath(AM_PathDump));
                        AM_AppendtoBinaryFile(Dumpfilename, pBuffer->GetDataPtr(), pBuffer->GetDataSize());
                    }
#endif

//test code, for fade in
#ifdef AM_DEBUG
                    AM_ASSERT(mpRecConfig);
                    if (mbNeedFadeIn) {
                        if (mpRecConfig && 0 == mpRecConfig->enable_fade_in) {
                            AMLOG_INFO("[fade in calculate]: is disabled.\n");
                            mbNeedFadeIn = false;
                        } else if (mFadeInCount < DMAX_FADE_CNT) {
                            AMLOG_DEBUG("[fade in calculate]: mFadeInCount %d, mFadeInBufferNumber %d, mFadeInMaxBufferNumber %d, pBuffer->GetDataPtr() %p, pBuffer->mDataSize/sizeof(s16) %d, shift %d,\n", mFadeInCount, mFadeInBufferNumber, mFadeInMaxBufferNumber, pBuffer->GetDataPtr(), pBuffer->mDataSize/sizeof(AM_S16), fade_level[mFadeInCount]);
                            _fade_audio_input_short((AM_S16*)pBuffer->GetDataPtr(), pBuffer->mDataSize/sizeof(AM_S16), fade_level[mFadeInCount]);
                            mFadeInBufferNumber ++;
                            if (mFadeInBufferNumber >= mFadeInMaxBufferNumber) {
                                mFadeInCount ++;
                                mFadeInBufferNumber = 0;
                            }
                        } else {
                            mbNeedFadeIn = false;
                            AMLOG_INFO("[fade in calculate]: finished.\n");
                        }
                    }
#endif

                    AMLOG_DEBUG("CFFMpegEncoder before encoding, pBuffer->mDataSize %d.\n", pBuffer->mDataSize);
                    encodeOutSize = avcodec_encode_audio(mpAudioEncoder, outData, pBuffer->mDataSize, inData);
                    AMLOG_DEBUG("CFFMpegEncoder after encoding, encodeOutSize %d.\n", encodeOutSize);

                    if (encodeOutSize > 0) {
                        pOutBuffer->SetType(CBuffer::DATA);
                        pOutBuffer->mpData = outData;
                        pOutBuffer->mFlags = 0;
                        pOutBuffer->mDataSize = encodeOutSize;
                        pOutBuffer->mBlockSize = encodeOutSize;
                        pOutBuffer->SetPTS(pBuffer->GetPTS());
                        AMLOG_DEBUG("CAudioEncoder outsize %d, PTS %lld\n", encodeOutSize, pBuffer->GetPTS());
                        pBuffer->Release();
                        pBuffer = NULL;

#ifdef AM_DEBUG
                        //dump coded data
                        if (mLogOutput & LogDumpSeparateBinary) {
                            AM_DumpBinaryFile_withIndex("audio_es", mDumpFileIndex - 1, pOutBuffer->GetDataPtr(), pOutBuffer->GetDataSize());
                        }
                        if (mLogOutput & LogDumpTotalBinary) {
                            AM_AppendtoBinaryFile("audio_es.total", pOutBuffer->GetDataPtr(), pOutBuffer->GetDataSize());
                        }
#endif
                        SendoutBuffer(pOutBuffer);
                        pOutBuffer->Release();
                        pOutBuffer = NULL;
                    } else if (encodeOutSize == 0) {
                        AMLOG_INFO("encodeOutSize == 0, maybe first audio packet.\n");
                        pBuffer->Release();
                        pBuffer = NULL;
                        msState = STATE_HAS_OUTPUTBUFFER;
                        break;
                    } else {   // drop this frame
                        AM_ERROR("Failed to encode data, ret %d.\n", encodeOutSize);
                        pBuffer->Release();
                        pBuffer = NULL;
                        msState = STATE_HAS_OUTPUTBUFFER;
                        break;
                    }
                    msState = STATE_IDLE;
                } else if (pBuffer->GetType() == CBuffer::EOS) {
                    AMLOG_INFO("CFFMpegEncoder get EOS, start sending EOS to downstreammer filters.\n");
                    pBuffer->Release();
                    pBuffer = NULL;
                    AMLOG_INFO("CFFMpegEncoder request outbuffer(for EOS) done.\n");
                    pOutBuffer->SetType(CBuffer::EOS);
                    SendoutBuffer(pOutBuffer);
                    pOutBuffer->Release();
                    pOutBuffer = NULL;
                    msState = STATE_PENDING;
                    AMLOG_INFO("CFFMpegEncoder send EOS end.\n");
                } else if (pBuffer->GetType() == CBuffer::FLOW_CONTROL) {
                    AMLOG_INFO("CFFMpegEncoder get FLOW_CONTROL buffer, start sending FLOW_CONTROL to downstreammer filters.\n");
                    pOutBuffer->SetType(CBuffer::FLOW_CONTROL);
                    pOutBuffer->SetFlags(pBuffer->GetFlags());
                    pBuffer->Release();
                    pBuffer = NULL;
                    SendoutBuffer(pOutBuffer);
                    pOutBuffer->Release();
                    pOutBuffer = NULL;
                    msState = STATE_IDLE;
                    AMLOG_INFO("CFFMpegEncoder get FLOW_CONTROL buffer end.\n");
                } else {
                    AM_ASSERT(0);
                    AM_ERROR("BAD Buffer type(%d) in CFFMpegEncoder::OnRun().\n", pBuffer->GetType());
                    pBuffer->Release();
                    pBuffer = NULL;
                    msState = STATE_HAS_OUTPUTBUFFER;
                }
                break;

            default:
                AM_ERROR("BAD STATE(%d) in CFFMpegEncoder OnRun.\n", msState);
            case STATE_PENDING:
            case STATE_ERROR:
                AM_ASSERT(!pBuffer);
                AM_ASSERT(!pOutBuffer);

                type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
                if (type == CQueue::Q_MSG) {
                    ProcessCmd(cmd);
                } else {
                    AM_ASSERT((CQueueInputPin*)result.pOwner == (CQueueInputPin*)mpInput);
                    if (!mpInput->PeekBuffer(pBuffer)) {
                        AM_ERROR("No buffer?\n");
                        break;
                    }
                    if (pBuffer->GetType() == CBuffer::EOS) {
                        if (!mpBufferPool->AllocBuffer(pOutBuffer, 0)) {
                            AM_ERROR("Failed to AllocBuffer\n");
                        } else {
                            pOutBuffer->SetType(CBuffer::EOS);
                            SendoutBuffer(pOutBuffer);
                            pOutBuffer->Release();
                            pOutBuffer = NULL;
                        }
                    } else if (pBuffer->GetType() == CBuffer::FLOW_CONTROL) {
                        pOutBuffer->SetType(CBuffer::FLOW_CONTROL);
                        pOutBuffer->SetFlags(pBuffer->GetFlags());
                        SendoutBuffer(pOutBuffer);
                        pOutBuffer->Release();
                        pOutBuffer = NULL;
                    } else {
                        AMLOG_WARN("CFFMpegEncoder: not processed buffer in STATE error/pending, buffer type %d.\n", pBuffer->GetType());
                    }
                    pBuffer->Release();
                    pBuffer = NULL;
                }
                break;
        }
    }

    AMLOG_INFO("CFFMpegEncoder::OnRun end");
}

#ifdef AM_DEBUG
void CFFMpegEncoder::PrintState()
{
    AMLOG_WARN("FFEncoder: msState=%d.\n", msState);
    AM_ASSERT(mpInput);
    AM_ASSERT(mpBufferPool);

    if (mpInput) {
        AMLOG_WARN(" inputpin have free data cnt %d.\n", mpInput->mpBufferQ->GetDataCnt());
    }
    if (mpBufferPool) {
        AMLOG_WARN(" buffer pool have %d free buffers.\n", mpBufferPool->GetFreeBufferCnt());
    }
}
#endif

//-----------------------------------------------------------------------
//
// CFFMpegEncoderInput
//
//-----------------------------------------------------------------------
CFFMpegEncoderInput *CFFMpegEncoderInput::Create(CFilter *pFilter)
{
	CFFMpegEncoderInput *result = new CFFMpegEncoderInput(pFilter);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CFFMpegEncoderInput::Construct()
{
	AM_ERR err = inherited::Construct(((CFFMpegEncoder*)mpFilter)->MsgQ());
	if (err != ME_OK)
		return err;
	return ME_OK;
}

//-----------------------------------------------------------------------
//
// CFFMpegEncoderOutput
//
//-----------------------------------------------------------------------
CFFMpegEncoderOutput* CFFMpegEncoderOutput::Create(CFilter *pFilter)
{
	CFFMpegEncoderOutput *result = new CFFMpegEncoderOutput(pFilter);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

