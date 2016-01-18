/*
 * video_encoder.cpp
 *
 * History:
 *    2011/7/18 - [QingXiong] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "audio_decoder_hw"
//#define AMDROID_DEBUG

#include "am_new.h"
#include "am_types.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "engine_guids.h"
#include "am_util.h"
#include "pbif.h"
#if PLATFORM_ANDROID
#include <basetypes.h>
#else
#include "basetypes.h"
#endif
extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
#include "aac_audio_dec.h"
}
#include "filter_list.h"

#include "audio_decoder_hw.h"


filter_entry g_audio_decoder_hw = {
	"AudioDecoderHW",
	CAudioDecoderHW::Create,
	NULL,
	CAudioDecoderHW::AcceptMedia,
};
au_aacdec_config_t au_aacdec_config;
AM_U32 CAudioDecoderHW::mpDecMem[106000];
AM_U8 CAudioDecoderHW::mpDecBackUp[252];
AM_U8 CAudioDecoderHW::mpInputBuf[16384];
AM_U32 CAudioDecoderHW::mpOutBuf[8192];
//-------------------------------------------------------------
//
// CAudioDecoderHW
//
//-------------------------------------------------------------
IFilter* CAudioDecoderHW::Create(IEngine * pEngine)
{
    CAudioDecoderHW* result = new CAudioDecoderHW(pEngine);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_INT CAudioDecoderHW::AcceptMedia(CMediaFormat & format)
{
    AM_PRINTF("g_GlobalCfg.mUsePrivateAudioDecLib %d.\n", g_GlobalCfg.mUsePrivateAudioDecLib);

    if (g_GlobalCfg.mUsePrivateAudioDecLib == 0) {
        return 0;
    }

    if (*format.pFormatType == GUID_Format_FFMPEG_Stream &&
        *format.pMediaType == GUID_Audio && *format.pSubType == GUID_Audio_AAC)
        return 1;
    return 0;
}

AM_ERR CAudioDecoderHW::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
    {
        AM_ERROR("CAudioDecoderHW ,inherited::Construct fail err = %d .\n", err);
        return err;
    }
    DSetModuleLogConfig(LogModuleAudioDecoderHW);
    if ((mpInput = CAudioHWInput::Create(this)) == NULL)
    {
        AM_ERROR("CAudioDecoderHW ,CAudioHWInput::Create fail.\n");
        return ME_NO_MEMORY;
    }

    if ((mpOutput = CAudioHWOutput::Create(this)) == NULL)
    {
        AM_ERROR("CAudioDecoderHW ,CAudioHWOutput::Create fail.\n");
        return ME_NO_MEMORY;
    }
    InitHWAttr();
    return ME_OK;
}


bool CAudioDecoderHW::ProcessCmd(CMD & cmd)
{
//    AM_ERR err = ME_OK;
    switch(cmd.code)
    {
    case CMD_STOP:
        mbRun = false;
        CmdAck(ME_OK);
        break;

    case CMD_PAUSE:
        if(msState != STATE_PENDING)
        {
            //DoPause();
            msOldState = msState;
            msState = STATE_PENDING;
        }
        //CmdAck(ME_OK);
        break;

    case CMD_RESUME:
        if(msState == STATE_PENDING)
        {
            //DoResume();
            msState = msOldState;
        }
        //CmdAck(ME_OK);
        break;

    case CMD_FLUSH:
        //DoFlush();//no api..
        if(mpBuffer)
        {
            mpBuffer->Release();
            mpBuffer = NULL;
        }
        msState = STATE_PENDING;
        CmdAck(ME_OK);
        break;

    case CMD_AVSYNC:
        CmdAck(ME_OK);
        break;

    case CMD_OBUFNOTIFY:
        if(msState == STATE_WAIT_OUTPUT)
        {
            msState = STATE_READY;
        }
        break;

    case CMD_SOURCE_FILTER_BLOCKED:
        break;

    case CMD_BEGIN_PLAYBACK:
        AM_ASSERT(msState == STATE_PENDING);
        msState = STATE_IDLE;
        break;

    default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }
    return 1;
}

void CAudioDecoderHW::OnRun()
{
    CmdAck(ME_OK);
    CQueue::QType type;
    CQueue::WaitResult result;
    CMD cmd;

    mpBufferPool = mpOutput->mpBufferPool;
    AM_ASSERT(mpBufferPool);
    mbRun = true;
    while(mbRun)
    {
        switch (msState)
        {
        case STATE_IDLE:
            type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
            if(type == CQueue::Q_MSG)
            {
                ProcessCmd(cmd);
            }else{
                if(ReadInputData() == ME_OK)
                {
                    msState = STATE_HAS_INPUTDATA;
                }
            }
            break;

        case STATE_HAS_INPUTDATA:
            AM_ASSERT(mpBuffer);
            if(mpBufferPool->GetFreeBufferCnt() > 0)
            {
                msState = STATE_READY;
            }else{
                msState = STATE_WAIT_OUTPUT;
            }
            break;

        case STATE_WAIT_OUTPUT:
            AM_ASSERT(mpBuffer);
            if(mpBufferPool->GetFreeBufferCnt() <= 0)
            {
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
            }else{
                msState = STATE_READY;
            }
            break;

        case STATE_READY:
            AM_ASSERT(mpBuffer);
            //if(IsDecoderReady() != ME_OK)
            //{
                //AM_INFO("IsDecoderReady() != ME_OK\n");
                //mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                //ProcessCmd(cmd);
            //}else{
            ProcessBuffer();
            break;

        case STATE_PENDING:
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            ProcessCmd(cmd);
            break;

        case STATE_ERROR:
            break;

        default:
            AM_ERROR(" %d",(AM_UINT)msState);
            break;
        }
    }
    AM_PRINTF("CAudioDecoderHW exit OnRun\n");
}

AM_ERR CAudioDecoderHW::ProcessBuffer()
{
    if(mpBuffer->GetType() == CBuffer::EOS)
    {
        ProcessEOS();
        return ME_OK;
    }
    AM_UINT outSize, inSize;
    AVPacket* pPacket = (AVPacket* )((AM_U8*)mpBuffer + sizeof(CBuffer));
    au_aacdec_config.dec_wptr = (AM_S32 *)mpOutBuf;

    if(pPacket->data[0] != 0xff || (pPacket->data[1] |0x0f) != 0xff)
    {
        AddAdtsHeader(pPacket);
        au_aacdec_config.interBufSize = inSize = pPacket->size + 7;
    }else{
        WriteBitBuf(pPacket, 0);
        au_aacdec_config.interBufSize = inSize = pPacket->size;
    }
    aacdec_set_bitstream_rp(&au_aacdec_config);

    aacdec_decode(&au_aacdec_config);

    if(au_aacdec_config.ErrorStatus != 0)
    {
        AM_INFO("Error State:%d!!!\n", au_aacdec_config.ErrorStatus);
        mpBuffer->Release();
        mpBuffer = NULL;
        msState = STATE_IDLE;
        return ME_OK;
    }
    AM_ASSERT(au_aacdec_config.consumedByte == inSize);
    AM_ASSERT((au_aacdec_config.frameCounter - mFrameNum) == 1);
    mFrameNum = au_aacdec_config.frameCounter;
    //AM_INFO("au consumedByte: %d, frameSize: %d, frameCounter:%d, size:%d\n", au_aacdec_config.consumedByte,au_aacdec_config.frameSize,  au_aacdec_config.frameCounter, pPacket->size);

    //Should always succ.
    if (!mpOutput->AllocBuffer(mpOutBuffer))
    {
        AM_ERROR("CAudioDecoderHW, Get output buffer fail!\n");
        //mbRun = false;
        mpBuffer->Release();
        mpBuffer = NULL;
        msState = STATE_IDLE;
        return ME_BUSY;
    }
    //length donot change, just 32 high 16bits to 16.
    Audio32iTo16i(au_aacdec_config.dec_wptr, (AM_S16*)mpOutBuffer->GetDataPtr(), au_aacdec_config.srcNumCh, au_aacdec_config.frameSize);
    outSize = au_aacdec_config.frameSize * au_aacdec_config.srcNumCh * 2; //THIS size is bytes!!!!
    mpOutBuffer->SetType(CBuffer::DATA);
    mpOutBuffer->mPTS = pPacket->pts;
    mpOutBuffer->SetDataSize(outSize);

    //set audio info
    ((CAudioBuffer*)mpOutBuffer)->sampleRate = au_aacdec_config.externalSamplingRate;
    ((CAudioBuffer*)mpOutBuffer)->numChannels = au_aacdec_config.outNumCh;
    ((CAudioBuffer*)mpOutBuffer)->sampleFormat = 1; //FORMAT_S16_LE

    //AM_INFO("CAudioDecoderHW, Send Out :%d Audio Data!\n", mpOutBuffer->GetDataSize());
    mpOutput->SendBuffer(mpOutBuffer);

    mpBuffer->Release();
    mpBuffer = NULL;
    msState = STATE_IDLE;
    return ME_OK;
}


AM_ERR CAudioDecoderHW::ProcessEOS()
{
    mpBuffer->Release();
    mpBuffer = NULL;
     //Should always succ.
    if (!mpOutput->AllocBuffer(mpOutBuffer))
    {
        AM_ERROR("CAudioDecoderHW, Get output buffer fail!\n");
        //mbRun = false;
        msState = STATE_IDLE;
        return ME_BUSY;
    }

    mpOutBuffer->SetType(CBuffer::EOS);
    mpOutBuffer->SetDataSize(0);
    mpOutBuffer->SetDataPtr(NULL);
    mpOutput->SendBuffer(mpOutBuffer);
    msState = STATE_IDLE;
    return ME_OK;
}

//------------------------------------------------------------------------------
/*
AM_ERR CAudioDecoderHW::AddSyncWord(AVPacket* pPacket)
{
    AM_INT size = pPacket->size;
    mpInputBuf[size] = 0xff;
    mpInputBuf[size+1] = 0xf9;
    mpInputBuf[size+2] = 0x4c;
    mpInputBuf[size+3] = 0x80;
    mpInputBuf[size+4] = (size>>3)&0xff;
    mpInputBuf[size+5] = (size&0x07)<<5;
    mpInputBuf[size+6] = 0xfc;
    return ME_OK;
}*/

AM_ERR CAudioDecoderHW::AddAdtsHeader(AVPacket* pPacket)
{
    AM_UINT size = pPacket->size +7;
    AM_U8 adtsHeader[7];
    adtsHeader[0] = 0xff;
    adtsHeader[1] = 0xf9;
    adtsHeader[2] = 0x4c;
    adtsHeader[3] = 0x80;          //fixed header end, the last two bits is the length .
    adtsHeader[4] = (size>>3)&0xff;
    adtsHeader[5] = (size&0x07)<<5;   //todo
    adtsHeader[6] = 0xfc;  //todo
    for (AM_UINT i=0; i < 7; i++)
    {
        mpInputBuf[i] = adtsHeader[i];
        //hBitBuf->cntBits += 8;
    }
    WriteBitBuf(pPacket, 7);
    return ME_OK;
}

void CAudioDecoderHW::Audio32iTo16i(AM_S32* bufin, AM_S16* bufout, AM_S32 ch, AM_S32 proc_size)
{
  AM_S32 i,j;
  AM_S32* bufin_ptr;
  AM_S16* bufout_ptr;

  bufin_ptr = bufin;
  bufout_ptr = bufout;
  for( i = 0 ; i < proc_size ; i++ ) {
    for( j = 0 ; j < ch ; j++ ) {
      *bufout_ptr = (*bufin_ptr)>>16;
      bufin_ptr++;
      bufout_ptr++;
    }
  }
}



//TODO
AM_ERR CAudioDecoderHW::WriteBitBuf(AVPacket* pPacket, int offset)
{
    memcpy(mpInputBuf+offset, pPacket->data, pPacket->size);
    return ME_OK;
}
//!TODO
//------------------------------------------------------------------------------
AM_ERR CAudioDecoderHW::SetupHWDecoder()
{
    aacdec_setup(&au_aacdec_config);
    aacdec_open(&au_aacdec_config);

    if (au_aacdec_config.ErrorStatus != 0)
    {
        AM_INFO("SetupHWDecoder Failed!!!\n\n");
    }
    au_aacdec_config.dec_rptr =  (AM_U8 *)mpInputBuf;
    return ME_OK;
}

AM_ERR CAudioDecoderHW::SetInputFormat(CMediaFormat* pFormat)
{
    //check
    //if();
    AM_ERR err;
    mpStream = (AVStream* )pFormat->format;
    err = SetupHWDecoder();
    if(err != ME_OK)
    {
        AM_ERROR("SetupHWDecoder Failed!\n");
        return ME_ERROR;
    }
    err = mpOutput->SetOutputFormat();
    if (err != ME_OK)
        return err;

    return ME_OK;
}

AM_ERR CAudioDecoderHW::ReadInputData()
{
    if(!mpInput->PeekBuffer(mpBuffer))
    {
        AM_INFO("!!PeekBuffer Failed!\n");
        return ME_ERROR;
    }
    return ME_OK;
}

AM_ERR CAudioDecoderHW::SendBuffer()
{
    return ME_OK;
}

AM_ERR CAudioDecoderHW::InitHWAttr()
{
    au_aacdec_config.externalSamplingRate = 48000;         /* !< external sampling rate for raw decoding */
    au_aacdec_config.bsFormat	= ADTS_BSFORMAT; /* !< ADTS_BSFORMAT : default Bitstream Type */
    au_aacdec_config.outNumCh            = 2;
    au_aacdec_config.externalBitrate      = 0;             /* !< external bitrate for loading the input buffer */
    au_aacdec_config.frameCounter         = 0;
    au_aacdec_config.errorCounter         = 0;
    au_aacdec_config.interBufSize         = 0;
    au_aacdec_config.codec_lib_mem_addr = (AM_U32* )mpDecMem; //libaac work mem.
    au_aacdec_config.codec_lib_backup_addr = (AM_U8* )mpDecBackUp; //multi way decoder support
    au_aacdec_config.codec_lib_backup_size = 0;
    au_aacdec_config.consumedByte = 0;
    return ME_OK;
}

void CAudioDecoderHW::GetInfo(INFO& info)
{
    inherited::GetInfo(info);
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = 1;
    info.nOutput = 1;
}

#ifdef AM_DEBUG
void CAudioDecoderHW::PrintState()
{
    AMLOG_INFO("CAudioDecoderHW: msState=%d, %d input data, %d free buffers.\n", msState, mpInput->mpBufferQ->GetDataCnt(), mpBufferPool->GetFreeBufferCnt());
}
#endif

void CAudioDecoderHW::Delete()
{
    AM_INFO("CAudioDecoderHW Delete()\n");
    inherited::Delete();
    AM_INFO("CAudioDecoderHW Delete Done()\n");
}

CAudioDecoderHW::~CAudioDecoderHW()
{
    AM_INFO("~CAudioDecoderHW.\n");
    AM_DELETE(mpInput);
    mpInput = NULL;
    AM_DELETE(mpOutput);
    mpOutput = NULL;
    //AM_RELEASE(mpBufferPool);
    AMLOG_DESTRUCTOR("~CAudioDecoderHW end.\n");
}
//-------------------------------------------------------------
//
// CAudioHWInput
//
//-------------------------------------------------------------
CAudioHWInput* CAudioHWInput::Create(CFilter *pFilter)
{
    CAudioHWInput* result = new CAudioHWInput(pFilter);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CAudioHWInput::Construct()
{
    AM_ERR err = inherited::Construct(((CAudioDecoderHW*)mpFilter)->MsgQ());
    if (err != ME_OK)
        return err;
    return ME_OK;
}

CAudioHWInput::~CAudioHWInput()
{
    AMLOG_INFO("~CAudioHWInput.\n");
}

AM_ERR CAudioHWInput::CheckMediaFormat(CMediaFormat *pFormat)
{
    return ((CAudioDecoderHW*)mpFilter)->SetInputFormat(pFormat);
}
//-------------------------------------------------------------
//
// CAudioHWOutput
//
//-------------------------------------------------------------
CAudioHWOutput* CAudioHWOutput::Create(CFilter *pFilter)
{
    CAudioHWOutput* result = new CAudioHWOutput(pFilter);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CAudioHWOutput::Construct()
{
    return ME_OK;
}

AM_ERR CAudioHWOutput::SetOutputFormat()
{
    AVStream* pStream = ((CAudioDecoderHW*)mpFilter)->mpStream;
    AVCodecContext* pCodec = pStream->codec;

    mMediaFormat.pMediaType = &GUID_Decoded_Audio;
    //mMediaFormat.pSubType = &GUID_Video_YUV420NV12;
    mMediaFormat.pFormatType = &GUID_Format_FFMPEG_Media;
    mMediaFormat.auSamplerate = pCodec->sample_rate;
    mMediaFormat.auChannels = pCodec->channels;
    mMediaFormat.auSampleFormat = pCodec->sample_fmt;
    mMediaFormat.isChannelInterleave = 0;
    return ME_OK;
}

CAudioHWOutput::~CAudioHWOutput()
{
    AMLOG_DESTRUCTOR("~CAudioHWOutput.\n");
}

