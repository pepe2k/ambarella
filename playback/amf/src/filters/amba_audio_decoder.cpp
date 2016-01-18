
/*
 * amba_audio_decoder.cpp
 *
 * History:
 *    2010/09/20 - [Zhi He] Create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "amba_audio_decoder"
//#define AMDROID_DEBUG

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"

#include "am_pbif.h"
#include "pbif.h"
#include "engine_guids.h"
#include "filter_list.h"
#include "am_util.h"

extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}

extern "C" {
#include "audio_typedef.h"
#include "pcm_decode.h"
}

#include "amba_audio_decoder.h"

//#define __use_hardware_timmer__
#ifdef __use_hardware_timmer__
#include "am_hwtimer.h"
#else
#include "sys/atomics.h"
#endif

filter_entry g_amba_audio_decoder = {
	"ambaAudioDecoder",
	CAmbaAudioDecoder::Create,
	NULL,
	CAmbaAudioDecoder::AcceptMedia,
};

//-----------------------------------------------------------------------
//
// CAmbaAudioDecoder
//
//-----------------------------------------------------------------------
IFilter *CAmbaAudioDecoder::Create(IEngine *pEngine)
{
    CAmbaAudioDecoder *result = new CAmbaAudioDecoder(pEngine);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

int CAmbaAudioDecoder::AcceptMedia(CMediaFormat& format)
{
//    AM_INFO("***AcceptMedia %p.\n", &format);
    if ((*format.pFormatType == GUID_Format_FFMPEG_Stream) && (*format.pMediaType == GUID_Audio) && (*format.pSubType == GUID_Audio_PCM)) {
        return 1;
    }
//    AM_INFO("***CAmbaAudioDecoder::AcceptMedia fail, pFormatType %d.\n", format.pFormatType == &GUID_Format_FFMPEG_Stream);
//    AM_INFO("pMediaType %d, pSubType %d.\n", format.pMediaType == &GUID_Audio, format.pSubType == &GUID_Audio_PCM);
    return 0;
}

AM_ERR CAmbaAudioDecoder::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    if ((mpInput = CAmbaAudioDecoderInput::Create(this)) == NULL)
        return ME_ERROR;

    if ((mpOutput = CAmbaAudioDecoderOutput::Create(this)) == NULL)
        return ME_ERROR;

    return ME_OK;
}

CAmbaAudioDecoder::~CAmbaAudioDecoder()
{
    AMLOG_DESTRUCTOR("~CAmbaAudioDecoder.\n");
    if (mpPCMDecoderContent) {
        free(mpPCMDecoderContent);
        mpPCMDecoderContent = NULL;
    }
    if (mpMiddleUsedBuffer) {
        delete [] mpMiddleUsedBuffer;
        mpMiddleUsedBuffer = NULL;
    }
    AM_DELETE(mpInput);
    AM_DELETE(mpOutput);
    AMLOG_DESTRUCTOR("~CAmbaAudioDecoder done.\n");
}

void CAmbaAudioDecoder::Delete()
{
    AMLOG_DESTRUCTOR("CAmbaAudioDecoder::Delete().\n");
    if (mpPCMDecoderContent) {
        free(mpPCMDecoderContent);
        mpPCMDecoderContent = NULL;
    }
    if (mpMiddleUsedBuffer) {
        delete [] mpMiddleUsedBuffer;
        mpMiddleUsedBuffer = NULL;
    }
    AM_DELETE(mpOutput);
    mpOutput = NULL;

    AMLOG_DESTRUCTOR("CAmbaAudioDecoder::Delete() before AM_DELETE(mpInput).\n");
    AM_DELETE(mpInput);
    mpInput = NULL;

    AMLOG_DESTRUCTOR("CAmbaAudioDecoder::Delete(), before inherited::Delete().\n");
    inherited::Delete();
    AMLOG_DESTRUCTOR("CAmbaAudioDecoder::Delete() done.\n");
}

//only one inputpin
bool CAmbaAudioDecoder::ReadInputData()
{
    AM_ASSERT(!mpBuffer);
    AM_ASSERT(!mpPacket);

    if (!mpInput->PeekBuffer(mpBuffer)) {
        AMLOG_ERROR("No buffer?\n");
        return false;
    }

    if (mpBuffer->GetType() == CBuffer::EOS) {
        AMLOG_INFO("CAmbaAudioDecoder %p get EOS.\n", this);
        SendEOS(mpOutput);
        msState = STATE_PENDING;
        return false;
    }

    mpPacket = (AVPacket*)((AM_U8*)mpBuffer + sizeof(CBuffer));
    return true;
}

bool CAmbaAudioDecoder::ProcessCmd(CMD& cmd)
{
    AMLOG_DEBUG("****CAmbaAudioDecoder::ProcessCmd, cmd.code %d.\n", cmd.code);
    switch (cmd.code) {
        case CMD_STOP:
            mbRun = false;
            AMLOG_DEBUG("****CAmbaAudioDecoder::ProcessCmd, STOP cmd.\n");
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
            msState = STATE_PENDING;
            if (mpBuffer) {
                if(mpOriDataptr) {
                    mpPacket->data = mpOriDataptr;
                    mpPacket->size += mDataOffset;
                    mpOriDataptr = NULL;
                    mDataOffset = 0;
                }
                mpBuffer->Release();
                mpBuffer = NULL;
                mpPacket = NULL;
            }
            CmdAck(ME_OK);
            break;
        case CMD_AVSYNC:
            CmdAck(ME_OK);
            break;
        default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }
    return false;
}

void CAmbaAudioDecoder::CopyPCMData()
{
    AM_ASSERT(!mbNeedDecode);
    AM_ASSERT(msState == STATE_READY);
    CBuffer *pOutputBuffer;
    bool processDone = mpPacket->size <= AVCODEC_MAX_AUDIO_FRAME_SIZE;
    AM_INT outSize = processDone ? mpPacket->size : AVCODEC_MAX_AUDIO_FRAME_SIZE;
	
    if (!mpOutput->AllocBuffer(pOutputBuffer)) {
        AMLOG_ERROR("CAmbaAudioDecoder::CopyPCMData, get output buffer fail, exit.\n");
        mbRun = false;
        return;
    }

    //get from AV_NOPTS_VALUE(avcodec.h), use (mpPacket->pts!=AV_NOPTS_VALUE) will have warnings, use (mpPacket->pts >= 0) judgement instead
    //convert to ms
    if (mpPacket->pts >= 0) {
        mTimestamp = (mpPacket->pts * mpStream->time_base.num * 1000) / mpStream->time_base.den;
        AMLOG_PTS("CAmbaAudioDecoder[audio] recieve a PTS, mpPacket->pts=%llu, mCurrInputTimestamp =%llu ms, num=%d, den=%d.\n",mpPacket->pts, mTimestamp, mpStream->time_base.num, mpStream->time_base.den);
    }

    ::memcpy((void*)pOutputBuffer->GetDataPtr(), (void*)mpPacket->data, outSize);
    pOutputBuffer->SetDataSize(outSize);
    pOutputBuffer->SetType(CBuffer::DATA);
    pOutputBuffer->mPTS = mTimestamp;

    //set audio info
    ((CAudioBuffer*)pOutputBuffer)->sampleRate = mpCodec->sample_rate;
    ((CAudioBuffer*)pOutputBuffer)->numChannels = mpCodec->channels;
    ((CAudioBuffer*)pOutputBuffer)->sampleFormat = mpCodec->sample_fmt;
    
    mpOutput->SendBuffer(pOutputBuffer);
    AMLOG_VERBOSE("CAmbaAudioDecoder[audio] send a decoded data done.\n");
    decodedFrame ++;

    if (!processDone) { //has remainning data in avpacket

        if(!mDataOffset) {
            //save original data ptr
            mpOriDataptr = mpPacket->data;
        }
        AMLOG_DEBUG("CAmbaAudioDecoder[audio] decoding, remain ES, decodedSize=0x%x Bytes, mpPacket->size=0x%x Bytes. mpPacket->data=%p.\n", mpPCMDecoderContent->consumed_byte*mpPCMDecoderContent->channel_in_num, mpPacket->size, mpPacket->data);

        mpPacket->data += outSize;
        mpPacket->size -= outSize;
        mDataOffset += outSize;
        //if has output buffer, continue to process remaining data
        if(!(mpBufferPool->GetFreeBufferCnt() > 0))
            msState = STATE_HAS_INPUTDATA;
        return;
    }
    
    if(mDataOffset) {
        AM_ASSERT(mpOriDataptr);
        AM_ASSERT(mDataOffset);
        //restore original data ptr
        mpPacket->data = mpOriDataptr;
        mpPacket->size = mDataOffset;
        mpOriDataptr = NULL;
        mDataOffset = 0 ;
    }
    
    mpBuffer->Release();
    mpBuffer = NULL;
    mpPacket = NULL;
    msState = STATE_IDLE;

}

void CAmbaAudioDecoder::ProcessData()
{
    AM_ASSERT(msState == STATE_READY);
    CBuffer *pOutputBuffer;
	
    if (!mpOutput->AllocBuffer(pOutputBuffer)) {
        AMLOG_ERROR("CAmbaAudioDecoder, get output buffer fail, exit.\n");
        mbRun = false;
        return;
    }

    //get from AV_NOPTS_VALUE(avcodec.h), use (mpPacket->pts!=AV_NOPTS_VALUE) will have warnings, use (mpPacket->pts >= 0) judgement instead
    //convert to ms
    if (mpPacket->pts >= 0) {
        mTimestamp = (mpPacket->pts * mpStream->time_base.num * 1000) / mpStream->time_base.den;
        AMLOG_PTS("CAmbaAudioDecoder[audio] recieve a PTS, mpPacket->pts=%llu, mCurrInputTimestamp =%llu ms, num=%d, den=%d.\n",mpPacket->pts, mTimestamp, mpStream->time_base.num, mpStream->time_base.den);
    }

    //output ptr
    if (!mbChannelInterlave) {
        mpPCMDecoderContent->frame_size = AVCODEC_MAX_AUDIO_FRAME_SIZE/(mpPCMDecoderContent->channel_out_num*(mpPCMDecoderContent->pcmdec_resolution>>3));
        mpPCMDecoderContent->pcmbuf_wptr = (u32*)pOutputBuffer->GetDataPtr();
    } else {
        //need use a middle buffer, assert buffer is avaiable
        AM_ASSERT(mpMiddleUsedBuffer);
        AM_ASSERT(mpMiddleUsedBuffer == (AM_U8*)mpPCMDecoderContent->pcmbuf_wptr);
    }

    //input ptr
    mpPCMDecoderContent->bitbuf_rptr = mpPacket->data;
    mpPCMDecoderContent->inter_buf_size = mpPacket->size;

    //change to native endian if needed
    if (mbChangeToNativeEndian) {
        ChangeToNativeEndian();
    }

    //debug info
    AMLOG_DEBUG("debug info(mpPCMDecoderContent): channel_in_num %d, channel_out_num %d.\n", mpPCMDecoderContent->channel_in_num, mpPCMDecoderContent->channel_out_num);
    AMLOG_DEBUG("                  inter_buf_size %d, resolution %d, pcmdec_resolution %d, frame_size %d.\n", mpPCMDecoderContent->inter_buf_size, mpPCMDecoderContent->resolution, mpPCMDecoderContent->pcmdec_resolution, mpPCMDecoderContent->frame_size);
    AMLOG_DEBUG("                  bitbuf_rptr %p, pcmbuf_wptr %p.\n", mpPCMDecoderContent->bitbuf_rptr, mpPCMDecoderContent->pcmbuf_wptr);

    //dump for debug
//    static AM_UINT debugCnt = 0;
//    char Dumpfilename[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN] = {0};
//    snprintf(Dumpfilename, sizeof(Dumpfilename), "%s/input/pcm", AM_GetPath(AM_PathDump));
//    AM_DumpBinaryFile_withIndex(Dumpfilename, debugCnt, mpPCMDecoderContent->bitbuf_rptr, mpPCMDecoderContent->inter_buf_size);
    
#ifdef __use_hardware_timmer__
    AM_UINT time = AM_get_hw_timer_count();
    DecodeData();
    totalTime += AM_hwtimer2us(AM_get_hw_timer_count() - time);
#else
    struct timeval tvbefore, tvafter;
    gettimeofday(&tvbefore,NULL);
    DecodeData();
    gettimeofday(&tvafter,NULL);
    totalTime += (tvafter.tv_sec -tvbefore.tv_sec)*1000000 + tvafter.tv_usec - tvbefore.tv_usec;
#endif

//    char Dumpfilename[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN] = {0};
//    snprintf(Dumpfilename, sizeof(Dumpfilename), "%s/output/pcm", AM_GetPath(AM_PathDump));
//    AM_DumpBinaryFile_withIndex(Dumpfilename, debugCnt++, (AM_U8*)mpPCMDecoderContent->pcmbuf_wptr, mpPCMDecoderContent->consumed_byte);

    //debug info
    AMLOG_DEBUG("debug info(mpPCMDecoderContent): consumed_byte %d, processed_byte %d.\n", mpPCMDecoderContent->consumed_byte, mpPCMDecoderContent->processed_byte);
    AMLOG_DEBUG("                  has_dec_out %d, decode_error %d.\n", mpPCMDecoderContent->has_dec_out, mpPCMDecoderContent->decode_error);

    if (!(decodedFrame & 0x1f) && decodedFrame) {
        AMLOG_PERFORMANCE("audio: [%d] decoded [%d] dropped frames' average = %d uS  \n", decodedFrame, droppedFrame, totalTime/(decodedFrame+droppedFrame));	
    }

    //decode error
    if (mpPCMDecoderContent->decode_error) {
        AMLOG_ERROR("encounter errors, au_pcmdec_decode, decode_error=%d.\n", mpPCMDecoderContent->decode_error);
        droppedFrame ++;
    } else {
        
        if (mpPCMDecoderContent->has_dec_out) {
            /*if a frame has been decoded, output it*/
            //AM_ASSERT(mpPCMDecoderContent->processed_byte);
            pOutputBuffer->SetType(CBuffer::DATA);
            //mpPCMDecoderContent->processed_byte is zero, walk around
            pOutputBuffer->SetDataSize(mpPCMDecoderContent->processed_byte? mpPCMDecoderContent->processed_byte : mpPCMDecoderContent->consumed_byte);
            pOutputBuffer->mPTS = mTimestamp;

            //set audio info
            ((CAudioBuffer*)pOutputBuffer)->sampleRate = mpCodec->sample_rate;
            ((CAudioBuffer*)pOutputBuffer)->numChannels = mpPCMDecoderContent->channel_out_num;
            ((CAudioBuffer*)pOutputBuffer)->sampleFormat = mpCodec->sample_fmt;

            //convert from channel interlave to sample interlave if needed
            if (mbChannelInterlave) {
                ConvertChannleInterlaveToSampleInterlave(pOutputBuffer);
            }
            
            mpOutput->SendBuffer(pOutputBuffer);
            AMLOG_VERBOSE("CAmbaAudioDecoder[audio] send a decoded data done.\n");
        } else {
            AMLOG_ERROR("out_size==0, %d?\n", mpPCMDecoderContent->processed_byte);
        }

        decodedFrame ++;

        if((mpPCMDecoderContent->consumed_byte) == (u32)mpPacket->size) { //decode done
            AMLOG_DEBUG("CAmbaAudioDecoder[audio] decoding,done, OutputSample=%d,  decodedSize=0x%x Bytes.\n",mpPCMDecoderContent->processed_byte/mpCodec->channels/((mpCodec->sample_fmt==SAMPLE_FMT_S16) ? 2 :1), 
            mpPCMDecoderContent->consumed_byte);

        } else { //has remainning data in avpacket

            if(!mDataOffset) {
                //save original data ptr
                mpOriDataptr = mpPacket->data;
            }
            AMLOG_DEBUG("CAmbaAudioDecoder[audio] decoding, remain ES, decodedSize=0x%x Bytes, mpPacket->size=0x%x Bytes. mpPacket->data=%p.\n", mpPCMDecoderContent->consumed_byte*mpPCMDecoderContent->channel_in_num, mpPacket->size, mpPacket->data);

            mpPacket->data += mpPCMDecoderContent->consumed_byte;
            mpPacket->size -= mpPCMDecoderContent->consumed_byte;
            mDataOffset += mpPCMDecoderContent->consumed_byte;
            //if has output buffer, continue to process remaining data
            if(!(mpBufferPool->GetFreeBufferCnt() > 0))
                msState = STATE_HAS_INPUTDATA;
            return;
        }
    }
    
    if(mDataOffset) {
        AM_ASSERT(mpOriDataptr);
        AM_ASSERT(mDataOffset);
        //restore original data ptr
        mpPacket->data = mpOriDataptr;
        mpPacket->size = mDataOffset;
        mpOriDataptr = NULL;
        mDataOffset = 0 ;
    }
    
    mpBuffer->Release();
    mpBuffer = NULL;
    mpPacket = NULL;
    msState = STATE_IDLE;
}

void CAmbaAudioDecoder::OnRun()
{
    SetupDecoder();
    
    CmdAck(ME_OK);

    //should convert from channel interlave to sample interlave
    if (mbChannelInterlave) {
        mpMiddleUsedBuffer = new AM_U8[AVCODEC_MAX_AUDIO_FRAME_SIZE];
        if (mpMiddleUsedBuffer) {
            mpPCMDecoderContent->frame_size = AVCODEC_MAX_AUDIO_FRAME_SIZE/(mpPCMDecoderContent->channel_out_num*(mpPCMDecoderContent->pcmdec_resolution>>3));
            mpPCMDecoderContent->pcmbuf_wptr = (u32*)mpMiddleUsedBuffer;
        }
    }

#ifdef __use_hardware_timmer__
    AM_open_hw_timer();
#endif

    totalTime = 0;
    decodedFrame = 0;
    droppedFrame = 0;
    mpBufferPool = mpOutput->mpBufferPool;
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
    mbRun = true;
    msState = STATE_IDLE;

    AMLOG_PRINTF("CAmbaAudioDecoder %p start OnRun loop.\n", this);

    while (mbRun) {  
        AMLOG_STATE("start switch, msState=%d, %d input data, %d free buffers.\n", msState, mpInput->mpBufferQ->GetDataCnt(), mpBufferPool->GetFreeBufferCnt());

        switch (msState) {

            case STATE_IDLE:
                if (mbPaused) {
                    msState = STATE_PENDING;
                    break;
                }
                if (mpBufferPool->GetFreeBufferCnt() > 0) {
                    msState = STATE_HAS_OUTPUTBUFFER;
                } else {
                    type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
                    if(type == CQueue::Q_MSG) {
                        ProcessCmd(cmd);
                    } else {
                        AM_ASSERT((CQueueInputPin*)result.pOwner == (CQueueInputPin*)mpInput);
                        if (ReadInputData()) {
                            msState = STATE_HAS_INPUTDATA;
                        }
                    }
                }
                break;

            case STATE_HAS_OUTPUTBUFFER:
                AM_ASSERT(mpBufferPool->GetFreeBufferCnt() > 0);
                type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
                if (type == CQueue::Q_MSG) {
                    ProcessCmd(cmd);
                } else {
                    AM_ASSERT((CQueueInputPin*)result.pOwner == (CQueueInputPin*)mpInput);
                    if (ReadInputData()) {
                        msState = STATE_READY;
                    }
                }
                break;

            case STATE_PENDING:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case STATE_HAS_INPUTDATA:
                AM_ASSERT(mpPacket);
                AM_ASSERT(mpBuffer);

                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case STATE_READY:
                AM_ASSERT(mpPacket);
                AM_ASSERT(mpBuffer); 

                if (mbNeedDecode) {
                    AMLOG_DEBUG("CAmbaAudioDecoder::ProcessData().\n");
                    ProcessData();
                    AMLOG_DEBUG("CAmbaAudioDecoder::ProcessData() done.\n");
                } else {
                    //walk around when audio library is not mature
                    AMLOG_DEBUG("CAmbaAudioDecoder::CopyPCMData().\n");
                    CopyPCMData();
                    AMLOG_DEBUG("CAmbaAudioDecoder::CopyPCMData() done.\n");
                }
                break;

            default:
                AM_ERROR(" %d",(AM_UINT)msState);
                break;			
        }
        AMLOG_STATE("CAmbaAudioDecoder %p end switch, msState=%d, mbRun = %d.\n", this, msState, mbRun);
    }
	
    //mpBufferPool->SetNotifyOwner(NULL);
    if(mpBuffer) {

        //restore original data ptr, ugly here
        if(mpOriDataptr) {
            mpPacket->data = mpOriDataptr;
            mpPacket->size += mDataOffset;
            mpOriDataptr = NULL;
            mDataOffset = 0;
        }

        mpBuffer->Release();
        mpBuffer = NULL;
    }

    AMLOG_INFO("CAmbaAudioDecoder %p OnRun exit, msState=%d.\n", this, msState);
}

bool CAmbaAudioDecoder::SendEOS(CAmbaAudioDecoderOutput *pPin)
{
    CBuffer *pBuffer;

    AMLOG_INFO("CAmbaAudioDecoder Send EOS\n");
    if(decodedFrame || droppedFrame)
        AMLOG_INFO("[%d] decoded [%d] dropped frames' average = %d uS  \n", decodedFrame, droppedFrame, totalTime/(decodedFrame+droppedFrame));

    if (!pPin->AllocBuffer(pBuffer))
        return false;
	
    pBuffer->SetType(CBuffer::EOS);
    pPin->SendBuffer(pBuffer);

    return true;
}

AM_ERR CAmbaAudioDecoder::SetInputFormat(CMediaFormat *pFormat)
{
    AM_ERR err;
    if (*pFormat->pFormatType != GUID_Format_FFMPEG_Stream)
        return ME_ERROR;

    if (*pFormat->pMediaType != GUID_Audio && *pFormat->pSubType != GUID_Audio_PCM)
        return ME_ERROR;
    
    //current only support PCM
    mDecType = eAuDecType_PCM;

    if ((err = SetupDecoder()) != ME_OK) {
        CloseDecoder();
        return err;
    }

    mpStream = (AVStream*)pFormat->format;
    mpCodec = (AVCodecContext*)mpStream->codec;

    //current limitation
    AM_ASSERT(mpCodec->channels <= 2);
    AM_ASSERT(mpCodec->sample_fmt == SAMPLE_FMT_U8 || mpCodec->sample_fmt == SAMPLE_FMT_S16 || mpCodec->sample_fmt == SAMPLE_FMT_S32);
    
    mFormat.reserved1 = pFormat->reserved1;

    mpPCMDecoderContent->resolution = av_get_bits_per_sample((CodecID)mFormat.reserved1);
    mpPCMDecoderContent->channel_in_num = (u32)mpCodec->channels;

    //android only get 8 or 16 resolution
    if (mpPCMDecoderContent->resolution != 8) {
        mpPCMDecoderContent->pcmdec_resolution = 16;
    }
    //android only get 2 channnels
    if (mpCodec->channels >2) {
        mpPCMDecoderContent->channel_out_num = 2;
    } else {
        mpPCMDecoderContent->channel_out_num = mpCodec->channels;
    }
    
    mpOutput->mMediaFormat.pMediaType = &GUID_Decoded_Audio;
    mpOutput->mMediaFormat.pFormatType = &GUID_Audio_PCM;
    mpOutput->mMediaFormat.auChannels = mpPCMDecoderContent->channel_out_num;
    mpOutput->mMediaFormat.auSampleFormat = mpCodec->sample_fmt;
    mpOutput->mMediaFormat.auSamplerate = (AM_UINT)mpCodec->sample_rate;

    // check pcm convertion is needed
    if ((pFormat->pSubType == &GUID_Audio_PCM) && (mpPCMDecoderContent->channel_out_num == mpPCMDecoderContent->channel_in_num) && !mbChannelInterlave && !mbChangeToNativeEndian && (mpCodec->sample_fmt == SAMPLE_FMT_U8 || mpCodec->sample_fmt == SAMPLE_FMT_S16)) {
        mbNeedDecode = false;
    }

    return ME_OK;
}

void CAmbaAudioDecoder::ChangeToNativeEndian()
{
    //TODO, convert data in mpPacket
    AM_ASSERT(mpPacket);
    
    AMLOG_ERROR("CAmbaAudioDecoder::ChangeToNativeEndian not implement yet.\n");
}

void CAmbaAudioDecoder::ConvertChannleInterlaveToSampleInterlave(CBuffer*& pOutputBuffer)
{
    //TODO, convert data from mpMiddleUsedBuffer mpMiddleUsedBuffer to OutputBuffer
    AM_ASSERT(mpMiddleUsedBuffer);
    AM_ASSERT(pOutputBuffer);
    
    AMLOG_ERROR("CAmbaAudioDecoder::ConvertChannleInterlaveToSampleInterlave not implement yet.\n");
}

AM_ERR CAmbaAudioDecoder::SetupDecoder()
{
    switch (mDecType) {
        case eAuDecType_PCM:
            AM_ASSERT(!mpPCMDecoderContent);
            if (!mpPCMDecoderContent) {
                mpPCMDecoderContent =(pcm_decode_cs_t*) malloc(sizeof(pcm_decode_cs_t));
                if (!mpPCMDecoderContent) {
                    AMLOG_ERROR("malloc pcm_decode_cs_t fail in CAmbaAudioDecoder::SetupDecoder.\n");
                    return ME_NO_MEMORY;
                }
            }
            au_pcmdec_setup(mpPCMDecoderContent);
            break;

        default:
            AMLOG_ERROR("CAmbaAudioDecoder::SetupDecoder, not expected mDecType %d.\n", mDecType);
            return ME_BAD_FORMAT;
    }
    return ME_OK;
}

AM_ERR CAmbaAudioDecoder::CloseDecoder()
{
    switch (mDecType) {
        case eAuDecType_PCM:
            AM_ASSERT(mpPCMDecoderContent);
            if (!mpPCMDecoderContent) {
                free(mpPCMDecoderContent);
                mpPCMDecoderContent = NULL;
            }
            break;

        default:
            AMLOG_ERROR("CAmbaAudioDecoder::CloseDecoder, not expected mDecType %d.\n", mDecType);
            return ME_BAD_FORMAT;
    }
    return ME_OK;
}

AM_ERR CAmbaAudioDecoder::DecodeData()
{
    switch (mDecType) {
        case eAuDecType_PCM:
            AM_ASSERT(mpPCMDecoderContent);
            au_pcmdec_decode(mpPCMDecoderContent);
            break;

        default:
            AMLOG_ERROR("CAmbaAudioDecoder::DecodeData, not expected mDecType %d.\n", mDecType);
            return ME_BAD_FORMAT;
    }

    return ME_OK;
}

#ifdef AM_DEBUG
void CAmbaAudioDecoder::PrintState()
{
    AMLOG_PRINTF("CAmbaAudioDecoder: msState=%d, %d input data, %d free buffers, need decode %d.\n", msState, mpInput->mpBufferQ->GetDataCnt(), mpBufferPool->GetFreeBufferCnt(), mbNeedDecode);
}
#endif

//-----------------------------------------------------------------------
//
// CAmbaAudioDecoderInput
//
//-----------------------------------------------------------------------
CAmbaAudioDecoderInput *CAmbaAudioDecoderInput::Create(CFilter *pFilter)
{
    CAmbaAudioDecoderInput *result = new CAmbaAudioDecoderInput(pFilter);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CAmbaAudioDecoderInput::Construct()
{
    AM_ERR err = inherited::Construct(((CAmbaAudioDecoder*)mpFilter)->MsgQ());
    if (err != ME_OK)
        return err;
    return ME_OK;
}

CAmbaAudioDecoderInput::~CAmbaAudioDecoderInput()
{
    AMLOG_DESTRUCTOR("~CAmbaAudioDecoderInput.\n");
}

AM_ERR CAmbaAudioDecoderInput::CheckMediaFormat(CMediaFormat *pFormat)
{
    return ((CAmbaAudioDecoder*)mpFilter)->SetInputFormat(pFormat);
}

//-----------------------------------------------------------------------
//
// CAmbaAudioDecoderOutput
//
//-----------------------------------------------------------------------
CAmbaAudioDecoderOutput *CAmbaAudioDecoderOutput::Create(CFilter *pFilter)
{
    CAmbaAudioDecoderOutput *result = new CAmbaAudioDecoderOutput(pFilter);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CAmbaAudioDecoderOutput::Construct()
{
    return ME_OK;
}

CAmbaAudioDecoderOutput::~CAmbaAudioDecoderOutput()
{
    AMLOG_DESTRUCTOR("~CAmbaAudioDecoderOutput.\n");
}


