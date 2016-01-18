/*
 * aac_encoder.cpp
 *
 * History:
 *    2011/07/12 - [Jay Zhang] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#define LOG_TAG "aac_encoder"
//#define AMDROID_DEBUG
//#define RECORD_TEST_FILE

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "record_if.h"
#include "engine_guids.h"

#include "aac_audio_enc.h"
#include "aac_encoder.h"

IFilter* CreateAACEncoder(IEngine * pEngine)
{
    return CAACEncoder::Create(pEngine);
}

IFilter* CAACEncoder::Create(IEngine * pEngine)
{
    CAACEncoder * result = new CAACEncoder(pEngine);

    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CAACEncoder::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
    	return err;

    if ((mpInput = CAACEncoderInput::Create(this)) == NULL)
    	return ME_NO_MEMORY;


    if ((mpBufferPool = CSimpleBufferPool::Create("AACEncoderBuffer",
        16, sizeof(CBuffer) + MAX_AUDIO_BUFFER_SIZE)) == NULL)
        return ME_NO_MEMORY;

#ifdef RECORD_TEST_FILE
    mpAFile = CFileWriter::Create();
    if (mpAFile == NULL)
        return ME_ERROR;
    err = mpAFile->CreateFile("aac_encoder.aac");
    printf("CreateFile ret %d\n", err);
    if (err != ME_OK)
    return err;
#endif

//    mpTmpBuf = new AM_U8[MAX_OUTPUT_FRAME_SIZE];
    mpTmpEncBuf = new AM_U8[AAC_LIB_TEMP_ENC_BUF_SIZE];

    return ME_OK;
}

CAACEncoder::~CAACEncoder()
{
    AM_DELETE(mpInput);
    mpInput = NULL;
    for (AM_INT i = 0; i < MAX_NUM_OUTPUT_PIN; i++) {
        AM_DELETE(mpOutput[i]);
        mpOutput[i] = NULL;
    }
    AM_DELETE(mpBufferPool);
    mpBufferPool = NULL;

#ifdef RECORD_TEST_FILE
    AM_DELETE(mpAFile);
#endif
//    delete [] mpTmpBuf;
    delete [] mpTmpEncBuf;
}

void CAACEncoder::GetInfo(INFO& info)
{
    info.nInput = 1;
    info.nOutput = 1;
    info.pName = "AACEncoder";
}

IPin* CAACEncoder::GetInputPin(AM_UINT index)
{
    if (index == 0)
        return mpInput;
    return NULL;
}

IPin* CAACEncoder::GetOutputPin(AM_UINT index)
{
    if (index < MAX_NUM_OUTPUT_PIN) {
        return (IPin*)mpOutput[index];
    }
    return NULL;
}

AM_ERR CAACEncoder::Stop()
{
    AM_INFO("=== CAACEncoder::Stop()\n");
    mbRunFlag = false;
    inherited::Stop();
    return ME_OK;
}

AM_ERR CAACEncoder::AddOutputPin(AM_UINT& index, AM_UINT type)
{
    AM_INT i;
    AM_ASSERT(type == IParameters::StreamType_Audio);
    if (type != IParameters::StreamType_Audio) {
        AM_ERROR("CFFMpegEncoder::AddOutputPin only support audio.\n");
        return ME_ERROR;
    }

    for (i = 0; i < MAX_NUM_OUTPUT_PIN; i++) {
        if (mpOutput[i] == NULL)
            break;
    }
    if ((mpOutput[i] = CAACEncoderOutput::Create(this)) == NULL)
        return ME_NO_MEMORY;

    mpOutput[i]->SetBufferPool(mpBufferPool);

    index = i;
    return ME_OK;
}

AM_ERR CAACEncoder::SetParameters(StreamType type, StreamFormat format, UFormatSpecific* param, AM_UINT index)
{
    AM_ASSERT(type == StreamType_Audio);
    AM_ASSERT(format == StreamFormat_AAC);
    mSampleRate = param->audio.sample_rate;
    mNumOfChannels = param->audio.channel_number;
    mBitRate= param->audio.bitrate;

    if (param->audio.sample_format == SampleFMT_S16)
        mBitsPerSample = 16;
    else
        mBitsPerSample = 8;

    AM_INFO("channel %d, samplerate  %d, bits %d\n", mNumOfChannels, mSampleRate, mBitsPerSample);
    return ME_OK;
}

AM_ERR CAACEncoder::OpenEncoder()
{
    au_aacenc_config.enc_mode          = 0;      // 0: AAC; 1: AAC_PLUS; 3: AACPLUS_PS;
    au_aacenc_config.sample_freq       = mSampleRate;
    au_aacenc_config.coreSampleRate    = mSampleRate;
    au_aacenc_config.Src_numCh         = mNumOfChannels;
    au_aacenc_config.Out_numCh         = mNumOfChannels;
    au_aacenc_config.tns               = 1;
    au_aacenc_config.ffType            = 't';
    au_aacenc_config.bitRate           = mBitRate; // AAC: 128000; AAC_PLUS: 64000; AACPLUS_PS: 40000;
    au_aacenc_config.quantizerQuality  = 1;     // 0 - low, 1 - high, 2 - highest
    au_aacenc_config.codec_lib_mem_adr = (u32*)mpTmpEncBuf;

    aacenc_setup(&au_aacenc_config);
    aacenc_open(&au_aacenc_config);

    return ME_OK;
}

void CAACEncoder::OnRun()
{
    CQueueInputPin *pPin = NULL;
    CBuffer *pInBuffer;
    CBuffer *pOutBuffer;
    AM_U8 *pOutData = NULL;
    AM_UINT encodeOutSize = 0;

    if (OpenEncoder() != ME_OK) {
        AM_ERROR("OpenEncoder fail in CAACEncoder::OnRun.\n");
        return CmdAck(ME_ERROR);
    }
    else CmdAck(ME_OK);

    mbRunFlag = true;
    while(true) {

	if (!WaitInputBuffer(pPin, pInBuffer)) {
		AM_ERROR("Failed to WaitInputBuffer\n");
		break;
	}

	if (!mbRunFlag) {
		pInBuffer->Release();
		break;
	}

        if (!mpOutput[0]->AllocBuffer(pOutBuffer))
            AM_ASSERT(0);

         if (pInBuffer->GetType() == CBuffer::DATA) {

            pOutData = (AM_U8*)((AM_U8*)pOutBuffer+sizeof(CBuffer));

            AM_ASSERT(pInBuffer->GetDataSize() == SINGLE_CHANNEL_MAX_AUDIO_BUFFER_SIZE*mNumOfChannels);

            // encode data
            au_aacenc_config.enc_rptr = (s32*)pInBuffer->GetDataPtr();
            au_aacenc_config.enc_wptr = pOutData;//mpTmpBuf;
            //library encode pcm to aac
            aacenc_encode(&au_aacenc_config);
            encodeOutSize = (au_aacenc_config.nBitsInRawDataBlock + 7) >> 3;
            //memcpy(pOutData, mpTmpBuf, encodeOutSize);
            AMLOG_DEBUG("CAACEncoder send data, data size %d, au_aacenc_config.nBitsInRawDataBlock %d.\n", pInBuffer->GetDataSize(), au_aacenc_config.nBitsInRawDataBlock);
#ifdef RECORD_TEST_FILE
            mpAFile->WriteFile(pOutData, encodeOutSize);
#endif
            //send data
            pOutBuffer->SetType(CBuffer::DATA);
            pOutBuffer->mpData = pOutData;
            pOutBuffer->mFlags = 0;
            pOutBuffer->mDataSize = encodeOutSize;
            pOutBuffer->SetPTS(pInBuffer->GetPTS());
            AMLOG_DEBUG("CAACEncoder send data, pts %lld\n", pInBuffer->GetPTS());
        }
        else if (pInBuffer->GetType() == CBuffer::EOS) {
            pOutBuffer->SetType(CBuffer::EOS);
            AM_INFO("AACEncoder send EOS\n");
        }
        else if (pInBuffer->GetType() == CBuffer::FLOW_CONTROL) {
            pOutBuffer->SetType(CBuffer::FLOW_CONTROL);
            pOutBuffer->SetFlags(pInBuffer->GetFlags());
        }
        else {
            AM_ASSERT(0);
        }

        for (AM_INT i = 0; i < MAX_NUM_OUTPUT_PIN; i++) {
            if (mpOutput[i] != NULL) {
                if ((i + 1 < MAX_NUM_OUTPUT_PIN) && (mpOutput[i+1] != NULL))
                    pOutBuffer->AddRef();
                mpOutput[i]->SendBuffer(pOutBuffer);
            }
        }
        pInBuffer->Release();
    }
}

//-----------------------------------------------------------------------
//
// CAACEncoderInput
//
//-----------------------------------------------------------------------
CAACEncoderInput *CAACEncoderInput::Create(CFilter *pFilter)
{
    CAACEncoderInput *result = new CAACEncoderInput(pFilter);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CAACEncoderInput::Construct()
{
    AM_ERR err = inherited::Construct(((CAACEncoder*)mpFilter)->MsgQ());
    if (err != ME_OK)
        return err;
    return ME_OK;
}

CAACEncoderInput::~CAACEncoderInput()
{
}

AM_ERR CAACEncoderInput::CheckMediaFormat(CMediaFormat *pFormat)
{
    //TODO
    return ME_OK;
}

//-----------------------------------------------------------------------
//
// CAACEncoderOutput
//
//-----------------------------------------------------------------------
CAACEncoderOutput* CAACEncoderOutput::Create(CFilter *pFilter)
{
    CAACEncoderOutput *result = new CAACEncoderOutput(pFilter);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CAACEncoderOutput::Construct()
{
    return ME_OK;
}


