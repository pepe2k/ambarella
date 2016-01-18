/*
 * audio_effecter.cpp
 *
 * History:
 *    2010/10/20 - [He Zhi] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "audio_effecter"
//#define AMDROID_DEBUG

#include "math.h"

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
#include "libavcodec/opt.h"
}

#include "audio_effecter.h"

//#define __use_hardware_timmer__
#ifdef __use_hardware_timmer__
#include "am_hwtimer.h"
#else
#include "sys/time.h"
#endif

filter_entry g_audio_effecter = {
    "audioEffecter",
    CAudioEffecter::Create,
    NULL,
    CAudioEffecter::AcceptMedia,
};

IFilter* CreateAudioEffecter(IEngine *pEngine)
{
    return CAudioEffecter::Create(pEngine);
}

static const AVOption _toptions[] = {{NULL, NULL, 0, FF_OPT_TYPE_FLAGS, 0, 0, 0, 0, NULL}};
static const char *_tcontext_to_name(void *ptr)
{
    return "audioresample";
}
static const AVClass _taudioresample_context_class = { "ReSampleContext", _tcontext_to_name, (const struct AVOption *)_toptions, 0, 0, 0};

static AM_UINT _getSize(AM_INT fmt)
{
    switch (fmt) {
        case SAMPLE_FMT_U8:
            return 1;
        case SAMPLE_FMT_S16:
            return 2;
        case SAMPLE_FMT_S32:
            return 4;
        case SAMPLE_FMT_FLT:
            return sizeof(float);
        case SAMPLE_FMT_DBL:
            return sizeof(double);
        case SAMPLE_FMT_NONE:
        case SAMPLE_FMT_NB:
        default:
            AM_ERROR("Error SampleFormat %d.\n", fmt);
            return 1;
    }
    AM_ERROR("Error SampleFormat %d.\n", fmt);
    return 1;
}

template <typename T>
static void _Mix2channel(T* pi1, T* pi2, T* po1, AM_UINT nSamples)
{
    while (nSamples > 3) {
        po1[0] = (pi1[0] + pi2[0]) /2;
        po1[1] = (pi1[1] + pi2[1]) /2;
        po1[2] = (pi1[2] + pi2[2]) /2;
        po1[3] = (pi1[3] + pi2[3]) /2;
        po1 += 4;
        pi1 += 4;
        pi2 += 4;
        nSamples -= 4;
    }
    while (nSamples > 0) {
        *po1++ = ((*pi1++) + (*pi2++))/2;
        nSamples --;
    }
}

//L, C, R, surrounding, LB, RB
//L = pi[0], C=pi[1], R=pi[2], sur=pi[3], LB=pi[4], RB=pi[5]
//L = L/2 + (C + sur)/8 + LB/4, R = R/2 + (C + sur)/8 + RB/4
template <typename T>
static void _Mix6channelsto2channelsSampeInterleave(T* pi, T* po, AM_UINT nSamples)
{
    while (nSamples > 0) {
        po[0] = pi[0]/2 + (pi[1] + pi[3])/8 + pi[4]/4;
        po[1] = pi[2]/2 + (pi[1] + pi[3])/8 + pi[5]/4;
        po += 2;
        pi += 6;
        nSamples --;
    }
}

//L, C, R, surrounding, LB, RB
//L = pi1, C=pi2, R=pi3, sur=pi4, LB=pi5, RB=pi6
//L = L/2 + (C + sur)/8 + LB/4, R = R/2 + (C + sur)/8 + RB/4
template <typename T>
static void _Mix6channelsto2channels(T* pi1, T* pi2, T* pi3, T* pi4, T* pi5, T* pi6, T* po1, T* po2, AM_UINT nSamples)
{
    while (nSamples > 3) {
        po1[0] = (pi1[0]/2) + (pi2[0] + pi4[0])/8 + pi5[0] /4;
        po2[0] = (pi3[0]/2) + (pi2[0] + pi4[0])/8 + pi6[0] /4;
        po1[1] = (pi1[1]/2) + (pi2[1] + pi4[1])/8 + pi5[1] /4;
        po2[1] = (pi3[1]/2) + (pi2[1] + pi4[1])/8 + pi6[1] /4;
        po1[2] = (pi1[2]/2) + (pi2[2] + pi4[2])/8 + pi5[2] /4;
        po2[2] = (pi3[2]/2) + (pi2[2] + pi4[2])/8 + pi6[2] /4;
        po1[3] = (pi1[3]/2) + (pi2[3] + pi4[3])/8 + pi5[3] /4;
        po2[3] = (pi3[3]/2) + (pi2[3] + pi4[3])/8 + pi6[3] /4;
        po1 += 4;
        po2 += 4;
        pi1 += 4;
        pi2 += 4;
        pi3 += 4;
        pi4 += 4;
        pi5 += 4;
        pi6 += 4;
        nSamples -= 4;
    }
    while (nSamples > 0) {
        po1[0] = (pi1[0]/2) + (pi2[0] + pi4[0])/8 + pi5[0] /4;
        po2[0] = (pi3[0]/2) + (pi2[0] + pi4[0])/8 + pi6[0] /4;
        po1 ++;
        po2 ++;
        pi1 ++;
        pi2 ++;
        pi3 ++;
        pi4 ++;
        pi5 ++;
        pi6 ++;
        nSamples --;
    }
}

template <typename T>
static void _Select2channel(T* pi1, T* po1, AM_UINT skipChannels, AM_UINT nSamples)
{
    while (nSamples > 0) {
        *po1++ = *pi1++;
        *po1++ = *pi1++;
        pi1 += skipChannels;
        nSamples --;
    }
}

template <typename T>
static void _Select1channel(T* pi1, T* po1, AM_UINT skipChannels, AM_UINT nSamples)
{
    while (nSamples > 0) {
        *po1++ = *pi1++;
        pi1 += skipChannels;
        nSamples --;
    }
}

template <typename T>
static void _Deinterleave2channel(T* pi1, T* po1, T* po2, AM_UINT skip_channels, AM_UINT nSamples)
{
    while (nSamples > 0) {
        *po1++ = *pi1++;
        *po2++ = *pi1++;
        pi1 += skip_channels;
        nSamples --;
    }
}

template <typename T>
static void _Deinterleave6channel(T* pi1, T* po1, T* po2, T* po3, T* po4, T* po5, T* po6, AM_UINT nSamples)
{
    while (nSamples > 0) {
        *po1++ = *pi1++;
        *po2++ = *pi1++;
        *po3++ = *pi1++;
        *po4++ = *pi1++;
        *po5++ = *pi1++;
        *po6++ = *pi1++;
        nSamples --;
    }
}

template <typename T>
static void _Deinterleave1channel(T* pi1, T* po1, AM_UINT skip_channels, AM_UINT nSamples)
{
    while (nSamples > 0) {
        *po1++ = *pi1++;
        pi1 += skip_channels;
        nSamples --;
    }
}

template <typename T>
static void _Interleave2channel(T* pi1, T* pi2, T* po1, AM_UINT nSamples)
{
    while (nSamples > 0) {
        *po1++ = *pi1++;
        *po1++ = *pi2++;
        nSamples --;
    }
}


//-----------------------------------------------------------------------
//
// CEffectElement
//
//-----------------------------------------------------------------------

CEffectElement::CEffectElement(const char* name):
    mbConfiged(false),
    mbSkip(false),
    mbCanShareBuffer(false)
{
    if (name) {
        memset(mpName,0,DElementNameLength + 1);
        strncpy(mpName, name, DElementNameLength);
    } else {
        memset(mpName, 0, DElementNameLength + 1);
    }
}

bool CEffectElement::SetParameter(SEffectParameter* pParam)
{
    AM_ASSERT(pParam);
    mEffectParameter = *pParam;
    return true;
}

void CEffectElement::QueryBufferInfo(AM_UINT& samplesize, AM_UINT& channels, AM_UINT& isChannelInterleave, AM_UINT& isSampleInterleave, bool isInput)
{
    AM_ASSERT(mbConfiged);
    if (isInput) {
        samplesize = _getSize(mInputFormat.sampleFormat);
        channels = mInputFormat.nChannels;
        isChannelInterleave = mInputFormat.isChannelInterleave;
        isSampleInterleave = mInputFormat.isSampleInterleave;
    } else {
        samplesize = _getSize(mOutputFormat.sampleFormat);
        channels = mOutputFormat.nChannels;
        isChannelInterleave = mOutputFormat.isChannelInterleave;
        isSampleInterleave = mInputFormat.isSampleInterleave;
    }
}

//-----------------------------------------------------------------------
//
// CFormatConvertor
//
//-----------------------------------------------------------------------
bool CFormatConvertor::ConfigInputOutputFormat(SEffectFormat* input, SEffectFormat* output)
{
    AM_ASSERT(input);
    AM_ASSERT(output);
    AM_ASSERT(input->nChannels == output->nChannels);
    AM_ASSERT(input->sampleRate == output->sampleRate);
    AM_ASSERT(input->isChannelInterleave == output->isChannelInterleave);
    AM_ASSERT(input->isSampleInterleave == output->isSampleInterleave);

    if (input->sampleFormat == output->sampleFormat) {
        mbSkip = true;
    }

    if (input->nChannels != output->nChannels || input->sampleRate != output->sampleRate || input->isChannelInterleave != output->isChannelInterleave || input->isSampleInterleave != output->isSampleInterleave) {
        AM_ERROR("CFormatConvertor input and output's format not match.\n");
        return false;
    }

    mInputFormat = *input;
    mOutputFormat = *output;
    mbConfiged = true;
    return true;
}

//convert sample format
//refer av_audio_convert in ffmpeg
AM_ERR CFormatConvertor::ConvertFormat(AM_U8 * in, AM_U8 * out, AM_UINT istride, AM_UINT ostride, AM_U8 * end)
{
    AM_ASSERT(mInputFormat.sampleFormat != mOutputFormat.sampleFormat);

#define CONV(ofmt, otype, ifmt, expr)\
if ((mOutputFormat.sampleFormat == ofmt) && (mInputFormat.sampleFormat == ifmt)) {\
    do{\
        *(otype*)out = expr; in += istride; out += ostride;\
    }while(out < end);\
}

    CONV(SAMPLE_FMT_S16, AM_S16, SAMPLE_FMT_U8 , (*(AM_U8*)in - 0x80)<<8)
    else CONV(SAMPLE_FMT_S32, AM_S32, SAMPLE_FMT_U8 , (*(AM_U8*)in - 0x80)<<24)
    else CONV(SAMPLE_FMT_FLT, float  , SAMPLE_FMT_U8 , (*(AM_U8*)in - 0x80)*(1.0 / (1<<7)))
    else CONV(SAMPLE_FMT_DBL, double , SAMPLE_FMT_U8 , (*(AM_U8*)in - 0x80)*(1.0 / (1<<7)))
    else CONV(SAMPLE_FMT_U8 , AM_U8, SAMPLE_FMT_S16, (*(AM_S16*)in>>8) + 0x80)
    else CONV(SAMPLE_FMT_S32, AM_S32, SAMPLE_FMT_S16,  *(AM_S16*)in<<16)
    else CONV(SAMPLE_FMT_FLT, float  , SAMPLE_FMT_S16,  *(AM_S16*)in*(1.0 / (1<<15)))
    else CONV(SAMPLE_FMT_DBL, double , SAMPLE_FMT_S16,  *(AM_S16*)in*(1.0 / (1<<15)))
    else CONV(SAMPLE_FMT_U8 , AM_U8, SAMPLE_FMT_S32, (*(AM_S32*)in>>24) + 0x80)
    else CONV(SAMPLE_FMT_S16, AM_S16, SAMPLE_FMT_S32,  *(AM_S32*)in>>16)
    else CONV(SAMPLE_FMT_FLT, float  , SAMPLE_FMT_S32,  *(AM_S32*)in*(1.0 / (1<<31)))
    else CONV(SAMPLE_FMT_DBL, double , SAMPLE_FMT_S32,  *(AM_S32*)in*(1.0 / (1<<31)))
    else CONV(SAMPLE_FMT_U8 , AM_U8, SAMPLE_FMT_FLT, av_clip_uint8(lrintf(*(float*)in * (1<<7)) + 0x80))
    else CONV(SAMPLE_FMT_S16, AM_S16, SAMPLE_FMT_FLT, av_clip_int16(lrintf(*(float*)in * (1<<15))))
    else CONV(SAMPLE_FMT_S32, AM_S32, SAMPLE_FMT_FLT, av_clipl_int32(llrintf(*(float*)in * (1<<31))))
    else CONV(SAMPLE_FMT_DBL, double , SAMPLE_FMT_FLT, *(float*)in)
    else CONV(SAMPLE_FMT_U8 , AM_U8, SAMPLE_FMT_DBL, av_clip_uint8(lrint(*(double*)in * (1<<7)) + 0x80))
    else CONV(SAMPLE_FMT_S16, AM_S16, SAMPLE_FMT_DBL, av_clip_int16(lrint(*(double*)in * (1<<15))))
    else CONV(SAMPLE_FMT_S32, AM_S32, SAMPLE_FMT_DBL, av_clipl_int32(llrint(*(double*)in * (1<<31))))
    else CONV(SAMPLE_FMT_FLT, float  , SAMPLE_FMT_DBL, *(double*)in)
    else {
        AM_ERROR("Error in/out sample format, %d %d.\n", mInputFormat.sampleFormat, mInputFormat.sampleFormat);
        return ME_ERROR;
    }

    return ME_OK;
}

AM_UINT CFormatConvertor::Process(SEffectBuffer* pInputBuffer, SEffectBuffer* pOutputBuffer, AM_UINT nSamples)
{
    AM_ASSERT(mbConfiged);
    AM_ASSERT(pInputBuffer);
    AM_ASSERT(pOutputBuffer);

    AM_UINT i = 0;
    AM_UINT istride = _getSize(mInputFormat.sampleFormat), ostride = _getSize(mOutputFormat.sampleFormat);
    AM_UINT consumed;

    AM_ASSERT((nSamples*istride) <= pInputBuffer->bufferSize);

    if (mbConfiged) {
        if (mInputFormat.isChannelInterleave) {
            consumed = nSamples*ostride;
            for (; i < mInputFormat.nChannels; i++) {
                ConvertFormat(pInputBuffer->pBuffer[i], pOutputBuffer->pBuffer[i], istride, ostride, pOutputBuffer->pBuffer[i] + consumed);
            }
        } else {
            consumed = mInputFormat.nChannels * nSamples * ostride;
            ConvertFormat(pInputBuffer->pBuffer[0], pOutputBuffer->pBuffer[0], istride, ostride, pOutputBuffer->pBuffer[0] + consumed);
        }
        AM_ASSERT(consumed <= pOutputBuffer->bufferSize);
        return nSamples;
    }

    AM_ERROR("not configed or configed fail, do nothing.\n");
    return 0;
}

//-----------------------------------------------------------------------
//
// CDeInterleaver
//
//-----------------------------------------------------------------------
bool CDeInterleaver::ConfigInputOutputFormat(SEffectFormat* input, SEffectFormat* output)
{
    AM_ASSERT(input);
    AM_ASSERT(output);
    AM_ASSERT(input->sampleFormat == output->sampleFormat);
    AM_ASSERT(input->nChannels == output->nChannels);
    AM_ASSERT(input->sampleRate == output->sampleRate);
    AM_ASSERT(!input->isChannelInterleave);
    AM_ASSERT(input->isSampleInterleave);
    AM_ASSERT(output->isChannelInterleave);
    AM_ASSERT(!output->isSampleInterleave);

    if (input->isChannelInterleave == output->isChannelInterleave && input->isSampleInterleave == output->isSampleInterleave) {
        mbSkip = true;
    }

    if (input->nChannels != output->nChannels || input->sampleRate != output->sampleRate || input->sampleFormat != output->sampleFormat) {
        AM_ERROR("CDeInterleaver input and output's format not match.\n");
        return false;
    }

    if (input->isChannelInterleave || !input->isSampleInterleave || !output->isChannelInterleave || output->isSampleInterleave) {
        AM_ERROR("1 CDeInterleaver input and output's format not match.\n");
        return false;
    }

    mInputFormat = *input;
    mOutputFormat = *output;
    mbConfiged = true;
    return true;
}

AM_UINT CDeInterleaver::Process(SEffectBuffer* pInputBuffer, SEffectBuffer* pOutputBuffer, AM_UINT nSamples)
{
    AM_ASSERT(mbConfiged);
    AM_ASSERT(pInputBuffer);
    AM_ASSERT(pOutputBuffer);

    AM_ASSERT(pInputBuffer->bufferNumber == 1);
    AM_ASSERT(pOutputBuffer->bufferNumber == mOutputFormat.nChannels);
    AM_ASSERT(mInputFormat.sampleFormat == mOutputFormat.sampleFormat);
    AM_ASSERT((nSamples*_getSize(mInputFormat.sampleFormat)) <= pOutputBuffer->bufferSize);
    AM_ASSERT((nSamples*_getSize(mInputFormat.sampleFormat)) <= pInputBuffer->bufferSize);

    if (mbConfiged) {
        if (mOutputFormat.nChannels == 2) {
            AM_ASSERT(mInputFormat.nChannels >= 2);
            if (mInputFormat.sampleFormat == SAMPLE_FMT_S16) {
                AM_S16* pi1 =(AM_S16*) pInputBuffer->pBuffer[0], *po1 = (AM_S16*) pOutputBuffer->pBuffer[0], *po2 = (AM_S16*)pOutputBuffer->pBuffer[1];
                _Deinterleave2channel(pi1, po1, po2, mInputFormat.nChannels - 2, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_U8) {
                AM_U8* pi1 =(AM_U8*) pInputBuffer->pBuffer[0], *po1 = (AM_U8*) pOutputBuffer->pBuffer[0], *po2 = (AM_U8*)pOutputBuffer->pBuffer[1];
                _Deinterleave2channel(pi1, po1, po2, mInputFormat.nChannels - 2, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_S32) {
                AM_S32* pi1 =(AM_S32*) pInputBuffer->pBuffer[0], *po1 = (AM_S32*) pOutputBuffer->pBuffer[0], *po2 = (AM_S32*)pOutputBuffer->pBuffer[1];
                _Deinterleave2channel(pi1, po1, po2, mInputFormat.nChannels - 2, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_FLT) {
                float* pi1 =(float*) pInputBuffer->pBuffer[0], *po1 = (float*) pOutputBuffer->pBuffer[0], *po2 = (float*)pOutputBuffer->pBuffer[1];
                _Deinterleave2channel(pi1, po1, po2, mInputFormat.nChannels - 2, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_DBL) {
                double* pi1 =(double*) pInputBuffer->pBuffer[0], *po1 = (double*) pOutputBuffer->pBuffer[0], *po2 = (double*)pOutputBuffer->pBuffer[1];
                _Deinterleave2channel(pi1, po1, po2, mInputFormat.nChannels - 2, nSamples);
            } else {
                AM_ERROR("error mInputFormat.sampleFormat %d in CDeInterleaver::Process.\n", mInputFormat.sampleFormat);
            }
        } else if (mOutputFormat.nChannels == 6) {
            AM_ASSERT(mInputFormat.nChannels >= 6);
            if (mInputFormat.sampleFormat == SAMPLE_FMT_S16) {
                AM_S16* pi1 =(AM_S16*) pInputBuffer->pBuffer[0];
                AM_S16* po1 = (AM_S16*) pOutputBuffer->pBuffer[0], *po2 = (AM_S16*)pOutputBuffer->pBuffer[1], *po3 = (AM_S16*) pOutputBuffer->pBuffer[2];
                AM_S16* po4 = (AM_S16*) pOutputBuffer->pBuffer[3], *po5 = (AM_S16*)pOutputBuffer->pBuffer[4], *po6 = (AM_S16*) pOutputBuffer->pBuffer[5];
                _Deinterleave6channel(pi1, po1, po2, po3, po4, po5, po6, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_U8) {
                AM_U8* pi1 =(AM_U8*) pInputBuffer->pBuffer[0];
                AM_U8* po1 = (AM_U8*) pOutputBuffer->pBuffer[0], *po2 = (AM_U8*)pOutputBuffer->pBuffer[1], *po3 = (AM_U8*) pOutputBuffer->pBuffer[2];
                AM_U8* po4 = (AM_U8*) pOutputBuffer->pBuffer[3], *po5 = (AM_U8*)pOutputBuffer->pBuffer[4], *po6 = (AM_U8*) pOutputBuffer->pBuffer[5];
                _Deinterleave6channel(pi1, po1, po2, po3, po4, po5, po6, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_S32) {
                AM_S32* pi1 =(AM_S32*) pInputBuffer->pBuffer[0];
                AM_S32* po1 = (AM_S32*) pOutputBuffer->pBuffer[0], *po2 = (AM_S32*)pOutputBuffer->pBuffer[1], *po3 = (AM_S32*) pOutputBuffer->pBuffer[2];
                AM_S32* po4 = (AM_S32*) pOutputBuffer->pBuffer[3], *po5 = (AM_S32*)pOutputBuffer->pBuffer[4], *po6 = (AM_S32*) pOutputBuffer->pBuffer[5];
                _Deinterleave6channel(pi1, po1, po2, po3, po4, po5, po6, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_FLT) {
                float* pi1 =(float*) pInputBuffer->pBuffer[0];
                float* po1 = (float*) pOutputBuffer->pBuffer[0], *po2 = (float*)pOutputBuffer->pBuffer[1], *po3 = (float*) pOutputBuffer->pBuffer[2];
                float* po4 = (float*) pOutputBuffer->pBuffer[3], *po5 = (float*)pOutputBuffer->pBuffer[4], *po6 = (float*) pOutputBuffer->pBuffer[5];
                _Deinterleave6channel(pi1, po1, po2, po3, po4, po5, po6, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_DBL) {
                double* pi1 = (double*) pInputBuffer->pBuffer[0];
                double* po1 = (double*) pOutputBuffer->pBuffer[0], *po2 = (double*)pOutputBuffer->pBuffer[1], *po3 = (double*) pOutputBuffer->pBuffer[2];
                double* po4 = (double*) pOutputBuffer->pBuffer[3], *po5 = (double*)pOutputBuffer->pBuffer[4], *po6 = (double*) pOutputBuffer->pBuffer[5];
                _Deinterleave6channel(pi1, po1, po2, po3, po4, po5, po6, nSamples);
            } else {
                AM_ERROR("error mInputFormat.sampleFormat %d in CDeInterleaver::Process.\n", mInputFormat.sampleFormat);
            }
        } else {
            AM_ERROR("not implement now, you can add implemention here, format %d channels %d.\n", mInputFormat.sampleFormat, mOutputFormat.nChannels);
            return 0;
        }
        return nSamples;
    }
    AM_ERROR("not configed or configed fail, do nothing.\n");
    return 0;
}

//-----------------------------------------------------------------------
//
// CDeInterleaverDownMixer
//
//-----------------------------------------------------------------------
bool CDeInterleaverDownMixer::ConfigInputOutputFormat(SEffectFormat* input, SEffectFormat* output)
{
    AM_ASSERT(input);
    AM_ASSERT(output);
    AM_ASSERT(input->sampleFormat == output->sampleFormat);
    AM_ASSERT(input->sampleRate == output->sampleRate);
    AM_ASSERT(!input->isChannelInterleave);
    AM_ASSERT(input->isSampleInterleave);
    AM_ASSERT(output->isChannelInterleave);
    AM_ASSERT(!output->isSampleInterleave);

    AM_ASSERT(input->nChannels != output->nChannels);

    if ((input->isChannelInterleave == output->isChannelInterleave || input->isSampleInterleave == output->isSampleInterleave) && input->nChannels == output->nChannels) {
        AM_ERROR("CDeInterleaverDownMixer skip?\n");
        mbSkip = true;
    }

    if (input->sampleRate != output->sampleRate || input->sampleFormat != output->sampleFormat) {
        AM_ERROR("CDeInterleaverDownMixer input and output's format not match.\n");
        return false;
    }

    if (input->isChannelInterleave || !input->isSampleInterleave || !output->isChannelInterleave || output->isSampleInterleave) {
        AM_ERROR("1 CDeInterleaverDownMixer input and output's format not match.\n");
        return false;
    }

    mInputFormat = *input;
    mOutputFormat = *output;
    mbConfiged = true;
    return true;
}

AM_UINT CDeInterleaverDownMixer::Process(SEffectBuffer* pInputBuffer, SEffectBuffer* pOutputBuffer, AM_UINT nSamples)
{
    AM_ASSERT(mbConfiged);
    AM_ASSERT(pInputBuffer);
    AM_ASSERT(pOutputBuffer);

    AM_ASSERT(pInputBuffer->bufferNumber == 1);
    AM_ASSERT(pOutputBuffer->bufferNumber == mOutputFormat.nChannels);
    AM_ASSERT(mInputFormat.sampleFormat == mOutputFormat.sampleFormat);
    AM_ASSERT((nSamples*_getSize(mInputFormat.sampleFormat)) <= pOutputBuffer->bufferSize);
    AM_ASSERT((nSamples*_getSize(mInputFormat.sampleFormat)) <= pInputBuffer->bufferSize);

    if (mbConfiged) {
        if (mOutputFormat.nChannels == 2) {
            AM_ASSERT(mInputFormat.nChannels >= 2);
            if (mInputFormat.sampleFormat == SAMPLE_FMT_S16) {
                AM_S16* pi1 =(AM_S16*) pInputBuffer->pBuffer[0], *po1 = (AM_S16*) pOutputBuffer->pBuffer[0], *po2 = (AM_S16*)pOutputBuffer->pBuffer[1];
                _Deinterleave2channel(pi1, po1, po2, mInputFormat.nChannels - 2, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_U8) {
                AM_U8* pi1 =(AM_U8*) pInputBuffer->pBuffer[0], *po1 = (AM_U8*) pOutputBuffer->pBuffer[0], *po2 = (AM_U8*)pOutputBuffer->pBuffer[1];
                _Deinterleave2channel(pi1, po1, po2, mInputFormat.nChannels - 2, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_S32) {
                AM_S32* pi1 =(AM_S32*) pInputBuffer->pBuffer[0], *po1 = (AM_S32*) pOutputBuffer->pBuffer[0], *po2 = (AM_S32*)pOutputBuffer->pBuffer[1];
                _Deinterleave2channel(pi1, po1, po2, mInputFormat.nChannels - 2, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_FLT) {
                float* pi1 =(float*) pInputBuffer->pBuffer[0], *po1 = (float*) pOutputBuffer->pBuffer[0], *po2 = (float*)pOutputBuffer->pBuffer[1];
                _Deinterleave2channel(pi1, po1, po2, mInputFormat.nChannels - 2, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_DBL) {
                double* pi1 =(double*) pInputBuffer->pBuffer[0], *po1 = (double*) pOutputBuffer->pBuffer[0], *po2 = (double*)pOutputBuffer->pBuffer[1];
                _Deinterleave2channel(pi1, po1, po2, mInputFormat.nChannels - 2, nSamples);
            } else {
                AM_ERROR("error mInputFormat.sampleFormat %d in CDeInterleaver::Process.\n", mInputFormat.sampleFormat);
            }
        } else {
            AM_ERROR("not implement now, you can add implemention here, format %d channels %d.\n", mInputFormat.sampleFormat, mOutputFormat.nChannels);
            return 0;
        }
        return nSamples;
    }
    AM_ERROR("not configed or configed fail, do nothing.\n");
    return 0;
}


//-----------------------------------------------------------------------
//
// CInterleaver
//
//-----------------------------------------------------------------------
bool CInterleaver::ConfigInputOutputFormat(SEffectFormat* input, SEffectFormat* output)
{
    AM_ASSERT(input);
    AM_ASSERT(output);
    AM_ASSERT(input->sampleFormat == output->sampleFormat);
    AM_ASSERT(input->nChannels == output->nChannels);
    AM_ASSERT(input->sampleRate == output->sampleRate);
    AM_ASSERT(input->isChannelInterleave);
    AM_ASSERT(!input->isSampleInterleave);
    AM_ASSERT(!output->isChannelInterleave);
    AM_ASSERT(output->isSampleInterleave);

    if (input->isChannelInterleave == output->isChannelInterleave || input->isSampleInterleave== output->isSampleInterleave) {
        AM_ERROR("CInterleaver skip?\n");
        mbSkip = true;
    }

    if (input->nChannels != output->nChannels || input->sampleRate != output->sampleRate || input->sampleFormat != output->sampleFormat) {
        AM_ERROR("CInterleaver input and output's format not match.\n");
        return false;
    }

    mInputFormat = *input;
    mOutputFormat = *output;
    mbConfiged = true;
    return true;
}

AM_UINT CInterleaver::Process(SEffectBuffer* pInputBuffer, SEffectBuffer* pOutputBuffer, AM_UINT nSamples)
{
    AM_ASSERT(mbConfiged);
    AM_ASSERT(pInputBuffer);
    AM_ASSERT(pOutputBuffer);

    AM_ASSERT(pOutputBuffer->bufferNumber == 1);
    AM_ASSERT(pInputBuffer->bufferNumber == mInputFormat.nChannels);
    AM_ASSERT(mInputFormat.sampleFormat == mOutputFormat.sampleFormat);
    AM_ASSERT((nSamples*_getSize(mOutputFormat.sampleFormat)) <= pOutputBuffer->bufferSize);
    AM_ASSERT((nSamples*_getSize(mInputFormat.sampleFormat)) <= pInputBuffer->bufferSize);

    if (mbConfiged) {
        if (mOutputFormat.nChannels == 2) {
            AM_ASSERT(mInputFormat.nChannels >= 2);
            if (mInputFormat.sampleFormat == SAMPLE_FMT_S16) {
                AM_S16* pi1 =(AM_S16*) pInputBuffer->pBuffer[0], *pi2 = (AM_S16*) pInputBuffer->pBuffer[1], *po1 = (AM_S16*)pOutputBuffer->pBuffer[0];
                _Interleave2channel(pi1, pi2, po1, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_U8) {
                AM_U8* pi1 =(AM_U8*) pInputBuffer->pBuffer[0], *pi2 = (AM_U8*) pInputBuffer->pBuffer[1], *po1 = (AM_U8*)pOutputBuffer->pBuffer[0];
                _Interleave2channel(pi1, pi2, po1, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_S32) {
                AM_S32* pi1 =(AM_S32*) pInputBuffer->pBuffer[0], *pi2 = (AM_S32*) pInputBuffer->pBuffer[1], *po1 = (AM_S32*)pOutputBuffer->pBuffer[0];
                _Interleave2channel(pi1, pi2, po1, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_FLT) {
                float* pi1 =(float*) pInputBuffer->pBuffer[0], *pi2 = (float*) pInputBuffer->pBuffer[1], *po1 = (float*)pOutputBuffer->pBuffer[0];
                _Interleave2channel(pi1, pi2, po1, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_DBL) {
                double* pi1 =(double*) pInputBuffer->pBuffer[0], *pi2 = (double*) pInputBuffer->pBuffer[1], *po1 = (double*)pOutputBuffer->pBuffer[0];
                _Interleave2channel(pi1, pi2, po1, nSamples);
            } else {
                AM_ERROR("error mInputFormat.sampleFormat %d in CDeInterleaver::Process.\n", mInputFormat.sampleFormat);
            }
        } else {
            AM_ERROR("not implement now, you can add implemention here, format %d channels %d.\n", mInputFormat.sampleFormat, mOutputFormat.nChannels);
            return 0;
        }
        return nSamples;
    }
    AM_ERROR("not configed or configed fail, do nothing.\n");
    return 0;
}

//-----------------------------------------------------------------------
//
// CDownMixer
//
//-----------------------------------------------------------------------
bool CDownMixer::ConfigInputOutputFormat(SEffectFormat* input, SEffectFormat* output)
{
    AM_ASSERT(input);
    AM_ASSERT(output);
    AM_ASSERT(input->sampleFormat == output->sampleFormat);
    AM_ASSERT(input->nChannels != output->nChannels);
    AM_ASSERT(input->sampleRate == output->sampleRate);
    //AM_ASSERT(input->isChannelInterleave);
    //AM_ASSERT(!input->isSampleInterleave);
    //AM_ASSERT(output->isChannelInterleave);
    //AM_ASSERT(!output->isSampleInterleave);

    if (input->nChannels == output->nChannels) {
        mbSkip = true;
    }

    if (input->sampleFormat != output->sampleFormat || input->sampleRate != output->sampleRate || input->sampleFormat != output->sampleFormat ) {
        AM_ERROR("CDownMixer input and output's format not match.\n");
        return false;
    }

    if (input->nChannels > output->nChannels && input->isChannelInterleave == output->isChannelInterleave && input->isSampleInterleave == output->isSampleInterleave) {
        mbCanShareBuffer = true;
    }

    mInputFormat = *input;
    mOutputFormat = *output;
    mbConfiged = true;
    return true;
}

AM_UINT CDownMixer::Process(SEffectBuffer* pInputBuffer, SEffectBuffer* pOutputBuffer, AM_UINT nSamples)
{
    AM_ASSERT(mbConfiged);
    AM_ASSERT(pInputBuffer);
    AM_ASSERT(pOutputBuffer);

    AM_ASSERT(mInputFormat.sampleFormat == mOutputFormat.sampleFormat);
    AM_ASSERT((nSamples*_getSize(mInputFormat.sampleFormat)) <= pOutputBuffer->bufferSize);
    AM_ASSERT((nSamples*_getSize(mInputFormat.sampleFormat)) <= pInputBuffer->bufferSize);

    if (mbConfiged) {
        if (mInputFormat.isChannelInterleave && mOutputFormat.isChannelInterleave) {
            AM_ASSERT(pOutputBuffer->bufferNumber == mOutputFormat.nChannels);
            AM_ASSERT(pInputBuffer->bufferNumber == mInputFormat.nChannels);
            if (mInputFormat.nChannels == 1) {
                AM_UINT i = 0;
                //copy 1 channel to other channel
                for (i = 0; i < mOutputFormat.nChannels; i++) {
                    if (pOutputBuffer->pBuffer[i] != pInputBuffer->pBuffer[i]) {
                        ::memcpy((void*)pOutputBuffer->pBuffer[i], pInputBuffer->pBuffer[0], nSamples*_getSize(mInputFormat.sampleFormat));
                    }
                }
            } else if (mInputFormat.nChannels == 2 && mOutputFormat.nChannels == 1) {
                //mix 2 channels to 1 channel
                if (mInputFormat.sampleFormat == SAMPLE_FMT_S16) {
                    AM_S16* po1 = (AM_S16*)pOutputBuffer->pBuffer[0];
                    AM_S16* pi1 = (AM_S16*)pInputBuffer->pBuffer[0];
                    AM_S16* pi2 = (AM_S16*)pInputBuffer->pBuffer[1];
                    _Mix2channel(pi1, pi2, po1, nSamples);
                } else if (mInputFormat.sampleFormat == SAMPLE_FMT_U8){
                    AM_U8* po1 = (AM_U8*)pOutputBuffer->pBuffer[0];
                    AM_U8* pi1 = (AM_U8*)pInputBuffer->pBuffer[0];
                    AM_U8* pi2 = (AM_U8*)pInputBuffer->pBuffer[1];
                    _Mix2channel(pi1, pi2, po1, nSamples);
                } else if (mInputFormat.sampleFormat == SAMPLE_FMT_S32) {
                    AM_S32* po1 = (AM_S32*)pOutputBuffer->pBuffer[0];
                    AM_S32* pi1 = (AM_S32*)pInputBuffer->pBuffer[0];
                    AM_S32* pi2 = (AM_S32*)pInputBuffer->pBuffer[1];
                    _Mix2channel(pi1, pi2, po1, nSamples);
                } else if (mInputFormat.sampleFormat == SAMPLE_FMT_FLT) {
                    float* po1 = (float*)pOutputBuffer->pBuffer[0];
                    float* pi1 = (float*)pInputBuffer->pBuffer[0];
                    float* pi2 = (float*)pInputBuffer->pBuffer[1];
                    _Mix2channel(pi1, pi2, po1, nSamples);
                } else if (mInputFormat.sampleFormat == SAMPLE_FMT_DBL) {
                    double* po1 = (double*)pOutputBuffer->pBuffer[0];
                    double* pi1 = (double*)pInputBuffer->pBuffer[0];
                    double* pi2 = (double*)pInputBuffer->pBuffer[1];
                    _Mix2channel(pi1, pi2, po1, nSamples);
                }
            } else if (mInputFormat.nChannels == 6 && mOutputFormat.nChannels == 2) {
                //mix 6 channels to 2 channel
                if (mInputFormat.sampleFormat == SAMPLE_FMT_S16) {
                    AM_S16* po1 = (AM_S16*)pOutputBuffer->pBuffer[0];
                    AM_S16* po2 = (AM_S16*)pOutputBuffer->pBuffer[1];
                    AM_S16* pi1 = (AM_S16*)pInputBuffer->pBuffer[0];
                    AM_S16* pi2 = (AM_S16*)pInputBuffer->pBuffer[1];
                    AM_S16* pi3 = (AM_S16*)pOutputBuffer->pBuffer[2];
                    AM_S16* pi4 = (AM_S16*)pInputBuffer->pBuffer[3];
                    AM_S16* pi5 = (AM_S16*)pInputBuffer->pBuffer[4];
                    AM_S16* pi6 = (AM_S16*)pInputBuffer->pBuffer[5];
                    _Mix6channelsto2channels(pi1, pi2, pi3, pi4, pi5, pi6, po1, po2, nSamples);
                } else if (mInputFormat.sampleFormat == SAMPLE_FMT_U8){
                    AM_U8* po1 = (AM_U8*)pOutputBuffer->pBuffer[0];
                    AM_U8* po2 = (AM_U8*)pOutputBuffer->pBuffer[1];
                    AM_U8* pi1 = (AM_U8*)pInputBuffer->pBuffer[0];
                    AM_U8* pi2 = (AM_U8*)pInputBuffer->pBuffer[1];
                    AM_U8* pi3 = (AM_U8*)pOutputBuffer->pBuffer[2];
                    AM_U8* pi4 = (AM_U8*)pInputBuffer->pBuffer[3];
                    AM_U8* pi5 = (AM_U8*)pInputBuffer->pBuffer[4];
                    AM_U8* pi6 = (AM_U8*)pInputBuffer->pBuffer[5];
                    _Mix6channelsto2channels(pi1, pi2, pi3, pi4, pi5, pi6, po1, po2, nSamples);
                } else if (mInputFormat.sampleFormat == SAMPLE_FMT_S32) {
                    AM_S32* po1 = (AM_S32*)pOutputBuffer->pBuffer[0];
                    AM_S32* po2 = (AM_S32*)pOutputBuffer->pBuffer[1];
                    AM_S32* pi1 = (AM_S32*)pInputBuffer->pBuffer[0];
                    AM_S32* pi2 = (AM_S32*)pInputBuffer->pBuffer[1];
                    AM_S32* pi3 = (AM_S32*)pOutputBuffer->pBuffer[2];
                    AM_S32* pi4 = (AM_S32*)pInputBuffer->pBuffer[3];
                    AM_S32* pi5 = (AM_S32*)pInputBuffer->pBuffer[4];
                    AM_S32* pi6 = (AM_S32*)pInputBuffer->pBuffer[5];
                    _Mix6channelsto2channels(pi1, pi2, pi3, pi4, pi5, pi6, po1, po2, nSamples);
                } else if (mInputFormat.sampleFormat == SAMPLE_FMT_FLT) {
                    float* po1 = (float*)pOutputBuffer->pBuffer[0];
                    float* po2 = (float*)pOutputBuffer->pBuffer[1];
                    float* pi1 = (float*)pInputBuffer->pBuffer[0];
                    float* pi2 = (float*)pInputBuffer->pBuffer[1];
                    float* pi3 = (float*)pOutputBuffer->pBuffer[2];
                    float* pi4 = (float*)pInputBuffer->pBuffer[3];
                    float* pi5 = (float*)pInputBuffer->pBuffer[4];
                    float* pi6 = (float*)pInputBuffer->pBuffer[5];
                    _Mix6channelsto2channels(pi1, pi2, pi3, pi4, pi5, pi6, po1, po2, nSamples);
                } else if (mInputFormat.sampleFormat == SAMPLE_FMT_DBL) {
                    double* po1 = (double*)pOutputBuffer->pBuffer[0];
                    double* po2 = (double*)pOutputBuffer->pBuffer[1];
                    double* pi1 = (double*)pInputBuffer->pBuffer[0];
                    double* pi2 = (double*)pInputBuffer->pBuffer[1];
                    double* pi3 = (double*)pOutputBuffer->pBuffer[2];
                    double* pi4 = (double*)pInputBuffer->pBuffer[3];
                    double* pi5 = (double*)pInputBuffer->pBuffer[4];
                    double* pi6 = (double*)pInputBuffer->pBuffer[5];
                    _Mix6channelsto2channels(pi1, pi2, pi3, pi4, pi5, pi6, po1, po2, nSamples);
                }
            } else {
                AM_ERROR("not implement now, you can add implemention here, format %d channels %d.\n", mInputFormat.sampleFormat, mOutputFormat.nChannels);
                return 0;
            }
            return nSamples;
        }else if (mInputFormat.isSampleInterleave && mOutputFormat.isSampleInterleave) {
            if (mInputFormat.nChannels == 6 && mOutputFormat.nChannels == 2) {
                //mix 6 channels to 2 channel
                //AM_INFO("downmix samples %d, mInputFormat.sampleFormat %d.\n", nSamples, mInputFormat.sampleFormat);
                if (mInputFormat.sampleFormat == SAMPLE_FMT_S16) {
                    AM_S16* po = (AM_S16*)pOutputBuffer->pBuffer[0];
                    AM_S16* pi = (AM_S16*)pInputBuffer->pBuffer[0];
                    _Mix6channelsto2channelsSampeInterleave(pi, po, nSamples);
                } else if (mInputFormat.sampleFormat == SAMPLE_FMT_U8) {
                    AM_U8* po = (AM_U8*)pOutputBuffer->pBuffer[0];
                    AM_U8* pi = (AM_U8*)pInputBuffer->pBuffer[0];
                    _Mix6channelsto2channelsSampeInterleave(pi, po, nSamples);
                } else if (mInputFormat.sampleFormat == SAMPLE_FMT_S32) {
                    AM_S32* po = (AM_S32*)pOutputBuffer->pBuffer[0];
                    AM_S32* pi = (AM_S32*)pInputBuffer->pBuffer[0];
                    _Mix6channelsto2channelsSampeInterleave(pi, po, nSamples);
                } else if (mInputFormat.sampleFormat == SAMPLE_FMT_FLT) {
                    float* po = (float*)pOutputBuffer->pBuffer[0];
                    float* pi = (float*)pInputBuffer->pBuffer[0];
                    _Mix6channelsto2channelsSampeInterleave(pi, po, nSamples);
                } else if (mInputFormat.sampleFormat == SAMPLE_FMT_DBL) {
                    double* po = (double*)pOutputBuffer->pBuffer[0];
                    double* pi = (double*)pInputBuffer->pBuffer[0];
                    _Mix6channelsto2channelsSampeInterleave(pi, po, nSamples);
                }
            } else {
                AM_ERROR(" sample interleave downmix not implement yet, input channels %d, output channels %d.\n", mInputFormat.nChannels, mOutputFormat.nChannels);
                return 0;
            }
            return nSamples;
        }
    }
    AM_ERROR("not configed or configed fail, do nothing.\n");
    return 0;
}

//-----------------------------------------------------------------------
//
// CSimpleDownMixer
//
//-----------------------------------------------------------------------
bool CSimpleDownMixer::ConfigInputOutputFormat(SEffectFormat* input, SEffectFormat* output)
{
    AM_ASSERT(input);
    AM_ASSERT(output);
    AM_ASSERT(input->sampleFormat == output->sampleFormat);
    AM_ASSERT(input->nChannels != output->nChannels);
    AM_ASSERT(input->sampleRate == output->sampleRate);
    AM_ASSERT(!input->isChannelInterleave);
    AM_ASSERT(!output->isChannelInterleave);

    if (input->nChannels == output->nChannels) {
        mbSkip = true;
    }

    if (input->sampleFormat != output->sampleFormat || input->sampleRate != output->sampleRate || input->sampleFormat != output->sampleFormat \
        || input->isChannelInterleave || output->isChannelInterleave) {
        AM_ERROR("CSimpleDownMixer input and output's format not match.\n");
        return false;
    }

    mInputFormat = *input;
    mOutputFormat = *output;
    mbConfiged = true;
    return true;
}

AM_UINT CSimpleDownMixer::Process(SEffectBuffer* pInputBuffer, SEffectBuffer* pOutputBuffer, AM_UINT nSamples)
{
    AM_ASSERT(mbConfiged);
    AM_ASSERT(pInputBuffer);
    AM_ASSERT(pOutputBuffer);

    //AM_ASSERT(pOutputBuffer->bufferNumber == mOutputFormat.nChannels);
    //AM_ASSERT(pInputBuffer->bufferNumber == mInputFormat.nChannels);
    AM_ASSERT(mInputFormat.sampleFormat == mOutputFormat.sampleFormat);
    AM_ASSERT((nSamples*_getSize(mInputFormat.sampleFormat)) <= pOutputBuffer->bufferSize);
    AM_ASSERT((nSamples*_getSize(mInputFormat.sampleFormat)) <= pInputBuffer->bufferSize);

    if (mbConfiged) {
        if (mInputFormat.nChannels > 2 && mOutputFormat.nChannels == 2) {
            //mix 2 channels to 1 channel
            if (mInputFormat.sampleFormat == SAMPLE_FMT_S16) {
                AM_S16* po1 = (AM_S16*)pOutputBuffer->pBuffer[0];
                AM_S16* pi1 = (AM_S16*)pInputBuffer->pBuffer[0];
                _Select2channel(pi1, po1, mInputFormat.nChannels - 2, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_U8){
                AM_U8* po1 = (AM_U8*)pOutputBuffer->pBuffer[0];
                AM_U8* pi1 = (AM_U8*)pInputBuffer->pBuffer[0];
                _Select2channel(pi1, po1, mInputFormat.nChannels - 2, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_S32) {
                AM_S32* po1 = (AM_S32*)pOutputBuffer->pBuffer[0];
                AM_S32* pi1 = (AM_S32*)pInputBuffer->pBuffer[0];
                _Select2channel(pi1, po1, mInputFormat.nChannels - 2, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_FLT) {
                float* po1 = (float*)pOutputBuffer->pBuffer[0];
                float* pi1 = (float*)pInputBuffer->pBuffer[0];
                _Select2channel(pi1, po1, mInputFormat.nChannels - 2, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_DBL) {
                double* po1 = (double*)pOutputBuffer->pBuffer[0];
                double* pi1 = (double*)pInputBuffer->pBuffer[0];
                _Select2channel(pi1, po1, mInputFormat.nChannels - 2, nSamples);
            }
        }else if (mInputFormat.nChannels > 1 && mOutputFormat.nChannels == 1){
            //mix 2 channels to 1 channel
            if (mInputFormat.sampleFormat == SAMPLE_FMT_S16) {
                AM_S16* po1 = (AM_S16*)pOutputBuffer->pBuffer[0];
                AM_S16* pi1 = (AM_S16*)pInputBuffer->pBuffer[0];
                _Select1channel(pi1, po1, mInputFormat.nChannels - 1, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_U8){
                AM_U8* po1 = (AM_U8*)pOutputBuffer->pBuffer[0];
                AM_U8* pi1 = (AM_U8*)pInputBuffer->pBuffer[0];
                _Select1channel(pi1, po1, mInputFormat.nChannels - 1, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_S32) {
                AM_S32* po1 = (AM_S32*)pOutputBuffer->pBuffer[0];
                AM_S32* pi1 = (AM_S32*)pInputBuffer->pBuffer[0];
                _Select1channel(pi1, po1, mInputFormat.nChannels - 1, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_FLT) {
                float* po1 = (float*)pOutputBuffer->pBuffer[0];
                float* pi1 = (float*)pInputBuffer->pBuffer[0];
                _Select1channel(pi1, po1, mInputFormat.nChannels - 1, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_DBL) {
                double* po1 = (double*)pOutputBuffer->pBuffer[0];
                double* pi1 = (double*)pInputBuffer->pBuffer[0];
                _Select1channel(pi1, po1, mInputFormat.nChannels - 1, nSamples);
            }
        } else if (mInputFormat.nChannels == 6 && mOutputFormat.nChannels == 2) {
            //mix 6 channels to 2 channel
            if (mInputFormat.sampleFormat == SAMPLE_FMT_S16) {
                AM_S16* po1 = (AM_S16*)pOutputBuffer->pBuffer[0];
                AM_S16* po2 = (AM_S16*)pOutputBuffer->pBuffer[1];
                AM_S16* pi1 = (AM_S16*)pInputBuffer->pBuffer[0];
                AM_S16* pi2 = (AM_S16*)pInputBuffer->pBuffer[1];
                AM_S16* pi3 = (AM_S16*)pOutputBuffer->pBuffer[2];
                AM_S16* pi4 = (AM_S16*)pInputBuffer->pBuffer[3];
                AM_S16* pi5 = (AM_S16*)pInputBuffer->pBuffer[4];
                AM_S16* pi6 = (AM_S16*)pInputBuffer->pBuffer[5];
                _Mix6channelsto2channels(pi1, pi2, pi3, pi4, pi5, pi6, po1, po2, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_U8){
                AM_U8* po1 = (AM_U8*)pOutputBuffer->pBuffer[0];
                AM_U8* po2 = (AM_U8*)pOutputBuffer->pBuffer[1];
                AM_U8* pi1 = (AM_U8*)pInputBuffer->pBuffer[0];
                AM_U8* pi2 = (AM_U8*)pInputBuffer->pBuffer[1];
                AM_U8* pi3 = (AM_U8*)pOutputBuffer->pBuffer[2];
                AM_U8* pi4 = (AM_U8*)pInputBuffer->pBuffer[3];
                AM_U8* pi5 = (AM_U8*)pInputBuffer->pBuffer[4];
                AM_U8* pi6 = (AM_U8*)pInputBuffer->pBuffer[5];
                _Mix6channelsto2channels(pi1, pi2, pi3, pi4, pi5, pi6, po1, po2, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_S32) {
                AM_S32* po1 = (AM_S32*)pOutputBuffer->pBuffer[0];
                AM_S32* po2 = (AM_S32*)pOutputBuffer->pBuffer[1];
                AM_S32* pi1 = (AM_S32*)pInputBuffer->pBuffer[0];
                AM_S32* pi2 = (AM_S32*)pInputBuffer->pBuffer[1];
                AM_S32* pi3 = (AM_S32*)pOutputBuffer->pBuffer[2];
                AM_S32* pi4 = (AM_S32*)pInputBuffer->pBuffer[3];
                AM_S32* pi5 = (AM_S32*)pInputBuffer->pBuffer[4];
                AM_S32* pi6 = (AM_S32*)pInputBuffer->pBuffer[5];
                _Mix6channelsto2channels(pi1, pi2, pi3, pi4, pi5, pi6, po1, po2, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_FLT) {
                float* po1 = (float*)pOutputBuffer->pBuffer[0];
                float* po2 = (float*)pOutputBuffer->pBuffer[1];
                float* pi1 = (float*)pInputBuffer->pBuffer[0];
                float* pi2 = (float*)pInputBuffer->pBuffer[1];
                float* pi3 = (float*)pOutputBuffer->pBuffer[2];
                float* pi4 = (float*)pInputBuffer->pBuffer[3];
                float* pi5 = (float*)pInputBuffer->pBuffer[4];
                float* pi6 = (float*)pInputBuffer->pBuffer[5];
                _Mix6channelsto2channels(pi1, pi2, pi3, pi4, pi5, pi6, po1, po2, nSamples);
            } else if (mInputFormat.sampleFormat == SAMPLE_FMT_DBL) {
                double* po1 = (double*)pOutputBuffer->pBuffer[0];
                double* po2 = (double*)pOutputBuffer->pBuffer[1];
                double* pi1 = (double*)pInputBuffer->pBuffer[0];
                double* pi2 = (double*)pInputBuffer->pBuffer[1];
                double* pi3 = (double*)pOutputBuffer->pBuffer[2];
                double* pi4 = (double*)pInputBuffer->pBuffer[3];
                double* pi5 = (double*)pInputBuffer->pBuffer[4];
                double* pi6 = (double*)pInputBuffer->pBuffer[5];
                _Mix6channelsto2channels(pi1, pi2, pi3, pi4, pi5, pi6, po1, po2, nSamples);
            }
        } else {
            AM_ERROR("not implement now, you can add implemention here, format %d channels %d.\n", mInputFormat.sampleFormat, mOutputFormat.nChannels);
            return 0;
        }
        return nSamples;
    }
    AM_ERROR("not configed or configed fail, do nothing.\n");
    return 0;
}


//-----------------------------------------------------------------------
//
// CSampleRateConvertor
//
//-----------------------------------------------------------------------
bool CSampleRateConvertor::ConfigInputOutputFormat(SEffectFormat* input, SEffectFormat* output)
{
    AM_ASSERT(input);
    AM_ASSERT(output);
    AM_ASSERT(input->sampleFormat == output->sampleFormat);
    AM_ASSERT(input->nChannels == output->nChannels);
    AM_ASSERT(input->sampleRate != output->sampleRate);
    AM_ASSERT(input->isChannelInterleave);
    AM_ASSERT(output->isChannelInterleave);

    if (input->sampleRate == output->sampleRate) {
        mbSkip = true;
    }

    if (input->sampleFormat != output->sampleFormat || input->nChannels != output->nChannels || input->sampleFormat != output->sampleFormat) {
        AM_ERROR("CSampleRateConvertor input and output's format not match.\n");
        return false;
    }

    if (input->sampleFormat!= SAMPLE_FMT_S16) {
        AM_ERROR("CSampleRateConvertor only support S16 yet.\n");
        return false;
    }

    //av_resample's parameter is proper here?
    mpResampler = av_resample_init((int)output->sampleRate, (int)input->sampleRate, 16, 10, 0, 0.8);
    *(const AVClass**)mpResampler = &_taudioresample_context_class;

    if(!mpResampler) {
        AM_ERROR("CSampleRateConvertor av_resample_init fail, out rate %d, in rate %d.\n", (int)output->sampleRate, (int)input->sampleRate);
        return false;
    }

    mInputFormat = *input;
    mOutputFormat = *output;
    mbConfiged = true;
    return true;
}

AM_UINT CSampleRateConvertor::Process(SEffectBuffer* pInputBuffer, SEffectBuffer* pOutputBuffer, AM_UINT nSamples)
{
    AM_UINT ret = 0;
    AM_ASSERT(mbConfiged);
    AM_ASSERT(pInputBuffer);
    AM_ASSERT(pOutputBuffer);

    AM_ASSERT(pOutputBuffer->bufferNumber == mOutputFormat.nChannels);
    AM_ASSERT(pInputBuffer->bufferNumber == mInputFormat.nChannels);
    AM_ASSERT(mInputFormat.sampleFormat == mOutputFormat.sampleFormat);
    AM_ASSERT((nSamples*_getSize(mInputFormat.sampleFormat)) <= pOutputBuffer->bufferSize);
    AM_ASSERT((nSamples*_getSize(mInputFormat.sampleFormat)) <= pInputBuffer->bufferSize);

    if (mbConfiged) {
        AM_ASSERT(mpResampler);
        AM_ASSERT(mInputFormat.sampleFormat== SAMPLE_FMT_S16);
        AM_UINT i = 0;
        AM_INT consumed;
        for (i = 0; i < mOutputFormat.nChannels; i++) {
            consumed = 0;
            AM_ASSERT(pInputBuffer->pBuffer[i] == (pInputBuffer->pBufferBase[i] + mReservedSamples[i]*_getSize(mInputFormat.sampleFormat)));
            //AM_INFO("start(%d) mReservedSamples %d, nSamples %d, p %p, base %p, diff %d.\n", i, mReservedSamples[i], nSamples, pInputBuffer->pBuffer[i], pInputBuffer->pBufferBase[i], pInputBuffer->pBuffer[i] - pInputBuffer->pBufferBase[i]);

            ret = av_resample(mpResampler, (short*)pOutputBuffer->pBuffer[i], (short*)pInputBuffer->pBufferBase[i], &consumed, (int)(nSamples + mReservedSamples[i]), (int)pOutputBuffer->bufferSize, (int)((i+1) == mOutputFormat.nChannels));
            //AM_INFO("in %p, out %p, ret %d, consumed %d.\n", (short*)pInputBuffer->pBuffer[i], (short*)pOutputBuffer->pBuffer[i], ret, consumed);

            //copy reserved data
            mReservedSamples[i] =(nSamples + mReservedSamples[i])  - ((AM_UINT)consumed);
            ::memcpy(pInputBuffer->pBufferBase[i], pInputBuffer->pBufferBase[i] + consumed*_getSize(mInputFormat.sampleFormat), mReservedSamples[i]*_getSize(mInputFormat.sampleFormat));
            pInputBuffer->pBuffer[i] = pInputBuffer->pBufferBase[i] + mReservedSamples[i]*_getSize(mInputFormat.sampleFormat);
            //AM_INFO("end(%d) mReservedSamples %d, nSamples %d.\n", i, mReservedSamples[i], nSamples);
            AM_ASSERT(mReservedSamples[i] < 80);//it should be less than 80? not very sure, it may cause buffer overflow? pay attention
        }
        return ret;
    }
    AM_ERROR("not configed or configed fail, do nothing.\n");
    return 0;
}

//-----------------------------------------------------------------------
//
// CAttachedBufferPool
//
//-----------------------------------------------------------------------

/*
void CAttachedBufferPool::OnReleaseBuffer(CBuffer *pBuffer)
{
    CSharedAudioBuffer* pSharedBuffer = (CSharedAudioBuffer*) pBuffer;
    if (pBuffer->GetType() == CBuffer::DATA && pSharedBuffer->mbAttached) {
        AM_ASSERT(mpAttachedBufferPool);
        AM_ASSERT(pSharedBuffer->mpCarrier);
        pSharedBuffer->mbAttached = false;
        mpAttachedBufferPool->Release((CBuffer*) (pSharedBuffer->mpCarrier));
    }
}
*/

CAttachedBufferPool *CAttachedBufferPool::Create(AM_UINT size, AM_UINT count)
{
	CAttachedBufferPool *result = new CAttachedBufferPool;
	if (result != NULL && result->Construct(size, count) != ME_OK)
	{
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAttachedBufferPool::Construct(AM_UINT size, AM_UINT count)
{
    AM_ERR err;

    if ((err = inherited::Construct(count)) != ME_OK)
        return err;

    if ((_pBuffers = new CSharedAudioBuffer[count]) == NULL)
    {
        AM_ERROR("Audio Effecter allocate CSharedAudioBuffer fail.\n");
        return ME_ERROR;
    }
    mpAttachedBufferPool = NULL;
    mnBuffers = count;
    return ME_OK;
}

CAttachedBufferPool::~CAttachedBufferPool()
{
    Detach();
    if(_pBuffers) {
        delete _pBuffers;
        _pBuffers = NULL;
    }
}

void CAttachedBufferPool::AttachToBufferPool(IBufferPool *pBufferPool)
{
    AM_UINT attached = 0;
    CBuffer* pBuffer = NULL, *pBuffer1 = NULL;
    AM_ERR err;

    CSharedAudioBuffer* pLocalBuffer = _pBuffers;
    pBufferPool->AddRef();
    if (mpAttachedBufferPool) {
        pBufferPool->Release();
        AMLOG_ERROR("already attached?\n");
        return;
    }

    mpAttachedBufferPool = pBufferPool;

    AM_ASSERT(mnBuffers == pBufferPool->GetFreeBufferCnt());
    AM_ASSERT(_pBuffers);
    AMLOG_INFO("attach pBufferPool->GetFreeBufferCnt()= %d, mnBuffers=%d.\n", pBufferPool->GetFreeBufferCnt(), mnBuffers);

    while (attached < mnBuffers) {
        //AMLOG_INFO("attached %d.\n", attached);
        pBufferPool->AllocBuffer(pBuffer, 0);
        AM_ASSERT(pBuffer);
        pLocalBuffer->mbAttached = true;
        pLocalBuffer->mpCarrier = (CAudioBuffer*)pBuffer;

        pBuffer1 = (CBuffer*)pLocalBuffer;
        pBuffer1->mpData = pBuffer->mpData;
        pBuffer1->mBlockSize = pBuffer->mBlockSize;
        pBuffer1->mpPool = this;
        ((CAudioBuffer*)pBuffer)->tag = (AM_INTPTR)pBuffer1;
        err = mpBufferQ->PostMsg(&pBuffer1, sizeof(pBuffer1));
        AM_ASSERT_OK(err);

        pLocalBuffer++;
        attached++;
    }

    AMLOG_INFO("attach done, attached=%d, mnBuffers=%d.\n", attached, mnBuffers);
    mnBuffers = attached;
    ((CActiveFilter*)mpNotifyOwner)->OutputBufferNotify((IBufferPool*)this);
}

void CAttachedBufferPool::Detach()
{
    CBuffer* pBuffer = NULL, *pBuffer1 = NULL;
//    AM_ERR err;

    if (!mpAttachedBufferPool || !_pBuffers) {
        AMLOG_INFO(" should be already detached? CAttachedBufferPool::Detach mpAttachedBufferPool %p _pBuffers %p?\n", mpAttachedBufferPool, _pBuffers);
        return;
    }

    CSharedAudioBuffer* pLocalBuffer = _pBuffers;

    while (mnBuffers) {

        if (pLocalBuffer->mbAttached == true) {
            AM_ASSERT(pLocalBuffer->mpCarrier);
            pBuffer = (CBuffer*)pLocalBuffer->mpCarrier;
            pBuffer1 = (CBuffer*)pLocalBuffer;
            pLocalBuffer->mbAttached = false;
            AM_ASSERT(((CAudioBuffer*)pBuffer)->tag == (AM_INTPTR)pBuffer1);
            mpAttachedBufferPool->Release(pBuffer);
        }

        pLocalBuffer++;
        mnBuffers--;
    }

    mnBuffers = 0;
    mpAttachedBufferPool->Release();
    mpAttachedBufferPool = NULL;
}

//-----------------------------------------------------------------------
//
// CAudioEffecter
//
//-----------------------------------------------------------------------
IFilter *CAudioEffecter::Create(IEngine *pEngine)
{
    CAudioEffecter *result = new CAudioEffecter(pEngine);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}



AM_INT CAudioEffecter::AcceptMedia(CMediaFormat& format)
{
#if TARGET_USE_AMBARELLA_A5S_DSP
//a5s not use this filter, for performance issue?
    return 0;
#endif
//    AM_INFO("***AcceptMedia %p.\n", &format);
    if (*format.pMediaType == GUID_Decoded_Audio && format.pFormatType == &GUID_Format_FFMPEG_Media) {
        return 1;
    }
//    AM_INFO("***CAudioEffecter::AcceptMedia fail, pFormatType %d.\n", format.pFormatType == &GUID_Format_FFMPEG_Stream);
//    AM_INFO("pMediaType %d, pSubType %d.\n", format.pMediaType == &GUID_Audio, format.pSubType == &GUID_Audio_PCM);
    return 0;
}

AM_ERR CAudioEffecter::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;
    DSetModuleLogConfig(LogModuleAudioEffecter);
    if ((mpInput = CAudioEffecterInput::Create(this)) == NULL)
        return ME_ERROR;

    if ((mpOutput = CAudioEffecterOutput::Create(this)) == NULL)
        return ME_ERROR;

    if ((mpAttachedBufferPool = CAttachedBufferPool::Create(0, 64)) == NULL)
        return ME_ERROR;

    mpInput->SetBufferPool(mpAttachedBufferPool);
    mReservedBuffers = 6;
    return ME_OK;
}

CAudioEffecter::~CAudioEffecter()
{
    AMLOG_DESTRUCTOR("CAudioEffecter::~CAudioEffecter().\n");
    mpAttachedBufferPool->Detach();
    ClearPipeline();
    AM_DELETE(mpOutput);
    mpOutput = NULL;

    AMLOG_DESTRUCTOR("CAudioEffecter::~CAudioEffecter() before AM_DELETE(mpInput).\n");
    AM_DELETE(mpInput);
    mpInput = NULL;

    AMLOG_DESTRUCTOR("CAudioEffecter::~CAudioEffecter() done.\n");
}

void CAudioEffecter::Delete()
{
    mpAttachedBufferPool->Detach();
    ClearPipeline();
    AMLOG_DESTRUCTOR("CAudioEffecter::Delete().\n");
    AM_DELETE(mpOutput);
    mpOutput = NULL;

    AMLOG_DESTRUCTOR("CAudioEffecter::Delete() before AM_DELETE(mpInput).\n");
    AM_DELETE(mpInput);
    mpInput = NULL;

    AMLOG_DESTRUCTOR("CAudioEffecter::Delete(), before inherited::Delete().\n");
    inherited::Delete();
    AMLOG_DESTRUCTOR("CAudioEffecter::Delete() done.\n");
}

//only one inputpin
bool CAudioEffecter::ReadInputData()
{
    AM_ASSERT(!mpInBuffer);

    if (!mpInput->PeekBuffer(mpInBuffer)) {
        AMLOG_ERROR("No buffer?\n");
        return false;
    }

    if (mpInBuffer->GetType() == CBuffer::EOS) {
        AMLOG_INFO("CAudioEffecter %p get EOS.\n", this);
        SendEOS(mpOutput);
        msState = STATE_PENDING;
        return false;
    }

    if (!mpInBuffer->GetDataSize()) {
        //no data in buffer
        mpInBuffer->Release();
        mpInBuffer = NULL;
        return false;
    }

    return true;
}

//convey output buffers to input buffers
void CAudioEffecter::ConveyBuffers()
{
    CBuffer* pOutBuffer;
    CBuffer* pInBuffer;
    AMLOG_VERBOSE("ConveyBuffers start, %d.\n", mpBufferPool->GetFreeBufferCnt());
    while (mpBufferPool->GetFreeBufferCnt() > mReservedBuffers) {
        if (!mpOutput->AllocBuffer(pOutBuffer))
            return;

        pInBuffer = (CBuffer*)(((CAudioBuffer*)pOutBuffer)->tag);
        AM_ASSERT(pInBuffer);
        AM_ASSERT(((CSharedAudioBuffer*)pInBuffer)->mbAttached == false);
        ((CSharedAudioBuffer*)pInBuffer)->mbAttached = true;
        AM_ASSERT(((CSharedAudioBuffer*)pInBuffer)->mpCarrier == pOutBuffer);
        ((CSharedAudioBuffer*)pInBuffer)->mpCarrier = (CAudioBuffer*)pOutBuffer;

        //map in/out size
        if (mGuessedOutSize == pOutBuffer->mBlockSize) {
            pInBuffer->mBlockSize = mGuessedInSize;
        } else {
            if (mBufferSizeM >= mBufferSizeN) {
                pInBuffer->mBlockSize = mGuessedOutSize = mGuessedInSize = pOutBuffer->mBlockSize;
            } else {
                mGuessedInSize = pInBuffer->mBlockSize = pOutBuffer->mBlockSize * mBufferSizeM/mBufferSizeN;
                mGuessedOutSize = pOutBuffer->mBlockSize;
            }
        }

        pInBuffer->Release();
    }
    AMLOG_VERBOSE("ConveyBuffers end, %d.\n", mpBufferPool->GetFreeBufferCnt());
}

void CAudioEffecter::PrepareInOutBuffer(CBuffer* pOutBuffer)
{
    AM_ASSERT(mpElementListHead);
    SEffectElementList* pEl = mpElementListHead->pNext;
    AM_ASSERT(pEl != mpElementListHead);

    pEl->inputBuffer.bufferNumber = 1;
    //input
    if (!mbPipelineInputBufferAllocated) {
        if (!mCurrentDataSize) {
            AM_ASSERT(mpInBuffer);
            //from input buffer
            pEl->inputBuffer.pBuffer[0] = mpInBuffer->GetDataPtr();
            mCurrentInSamples = mpInBuffer->GetDataSize()/_getSize(mpInput->mMediaFormat.auSampleFormat)/mpInput->mMediaFormat.auChannels;
            if (mMaxInSamples >= mCurrentInSamples) {
                pEl->inputBuffer.bufferSize = mpInBuffer->GetDataSize();
            } else {
                pEl->inputBuffer.bufferSize = mMaxInSamples*_getSize(mpInput->mMediaFormat.auSampleFormat)*mpInput->mMediaFormat.auChannels;
                //copy remaining data
                AM_ASSERT(mEffectFlag & DEffect_SampleRateConvertor); //should have sample rate convertor
                AM_ASSERT(mpReservedBufferBase); //should have reserved buffer
                mpCurrentPtr = mpReservedBufferBase;
                mCurrentDataSize = mpInBuffer->GetDataSize() - pEl->inputBuffer.bufferSize;
                AM_ASSERT(mCurrentDataSize <= AVCODEC_MAX_AUDIO_FRAME_SIZE);
                ::memcpy(mpCurrentPtr, (void*)(mpInBuffer->GetDataPtr() + pEl->inputBuffer.bufferSize), mCurrentDataSize);
                mCurrentInSamples = mMaxInSamples;
            }
        } else {
            //process remaining data first
            AM_ASSERT(mpReservedBufferBase);
            AM_ASSERT(mpCurrentPtr);
            AM_ASSERT(mCurrentDataSize <= AVCODEC_MAX_AUDIO_FRAME_SIZE);
            pEl->inputBuffer.pBuffer[0] = mpCurrentPtr;
            if (mMaxInSamples < (mCurrentDataSize/_getSize(mpInput->mMediaFormat.auSampleFormat)/mpInput->mMediaFormat.auChannels)) {
                pEl->inputBuffer.bufferSize = mMaxInSamples*_getSize(mpInput->mMediaFormat.auSampleFormat)*mpInput->mMediaFormat.auChannels;
                //copy remaining data
                AM_ASSERT(mEffectFlag & DEffect_SampleRateConvertor); //should have sample rate convertor
                mCurrentDataSize -= mMaxInSamples*_getSize(mpInput->mMediaFormat.auSampleFormat)*mpInput->mMediaFormat.auChannels;
                ::memcpy(mpCurrentPtr, (void*)mpCurrentPtr, mCurrentDataSize);
                mpCurrentPtr += mCurrentDataSize;
                mCurrentInSamples = mMaxInSamples;
            } else {
                pEl->inputBuffer.bufferSize = mCurrentDataSize;
                mCurrentInSamples = mCurrentDataSize/_getSize(mpInput->mMediaFormat.auSampleFormat)/mpInput->mMediaFormat.auChannels;
                mCurrentDataSize = 0;
            }
        }
    } else {
        AMLOG_DEBUG("pipeline's input buffer is allocted, first element is sample rate convertor?\n");
        AM_ASSERT(pEl->type == eSampleRateConvertor);
        //copy data to pipeline input data
        AM_UINT cpSize = mMaxInSamples*_getSize(mpInput->mMediaFormat.auSampleFormat)*mpInput->mMediaFormat.auChannels;
        if (!mCurrentDataSize) {
            AM_ASSERT(mpInBuffer);
            AM_ASSERT(mpInput->mMediaFormat.auChannels == 1);
            //from input buffer
            if (cpSize < mpInBuffer->GetDataSize()) {
                ::memcpy(pEl->inputBuffer.pBuffer[0], mpInBuffer->GetDataPtr(), cpSize);
                mpCurrentPtr = mpReservedBufferBase; mCurrentDataSize = mpInBuffer->GetDataSize() - cpSize;
                ::memcpy(mpCurrentPtr, mpInBuffer->GetDataPtr() + cpSize, mCurrentDataSize);
                pEl->inputBuffer.bufferSize= cpSize;
                mCurrentInSamples = mMaxInSamples;
            } else {
                ::memcpy(pEl->inputBuffer.pBuffer[0], mpInBuffer->GetDataPtr(), mpInBuffer->GetDataSize());
                pEl->inputBuffer.bufferSize= mpInBuffer->GetDataSize();
                mCurrentInSamples = mpInBuffer->GetDataSize()/_getSize(mpInput->mMediaFormat.auSampleFormat)/mpInput->mMediaFormat.auChannels;
            }
        } else {
            //process remaining data first
            AM_ASSERT(mpReservedBufferBase);
            AM_ASSERT(mpCurrentPtr);
            AM_ASSERT(mCurrentDataSize <= AVCODEC_MAX_AUDIO_FRAME_SIZE);
            if (cpSize < mCurrentDataSize) {
                ::memcpy(pEl->inputBuffer.pBuffer[0], mpCurrentPtr, cpSize);
                mpCurrentPtr += cpSize;
                mCurrentDataSize -= cpSize;
                pEl->inputBuffer.bufferSize = cpSize;
                mCurrentInSamples = mMaxInSamples;
            } else {
                ::memcpy(pEl->inputBuffer.pBuffer[0], mpCurrentPtr, mCurrentDataSize);
                mCurrentInSamples = mCurrentDataSize/_getSize(mpInput->mMediaFormat.auSampleFormat)/mpInput->mMediaFormat.auChannels;
                pEl->inputBuffer.bufferSize = mCurrentDataSize;
                mCurrentDataSize = 0;
            }
        }
    }

    //output
    mpElementListHead->inputBuffer.bufferSize = pOutBuffer->mBlockSize;
    mpElementListHead->inputBuffer.pBuffer[0] = pOutBuffer->GetDataPtr();
    mpElementListHead->inputBuffer.bufferNumber = 1;

    pEl = pEl->pNext;
    while (pEl != mpElementListHead) {
        AM_ASSERT(pEl->pElement);
        if (!pEl->bInputAllocated) {
            pEl->inputBuffer = pEl->pPre->inputBuffer;
        }
        pEl = pEl->pNext;
    }
}

void CAudioEffecter::UpdateFormat()
{
    AM_ASSERT(mpInBuffer);
    AM_ASSERT(!mpElementListHead);
    mpElementListTail = mpElementListHead = AddNewNode(NULL);//create head
    AM_ASSERT(mpElementListHead);
    mnEffects = 0;

    //setup in/out format, all in mpElementListHead;
    if (((CAudioBuffer*)mpInBuffer)->numChannels > 1) {
        mpOutput->mMediaFormat.isChannelInterleave = mpInput->mMediaFormat.isChannelInterleave = mpElementListHead->outputFormat.isChannelInterleave = 0 ;
    } else if (((CAudioBuffer*)mpInBuffer)->numChannels == 1){
        mpOutput->mMediaFormat.isChannelInterleave = mpInput->mMediaFormat.isChannelInterleave = mpElementListHead->outputFormat.isChannelInterleave = 1 ;
    } else {
        AM_ASSERT(0);
        AMLOG_ERROR("wrong nchannels %d.\n", ((CAudioBuffer*)mpInBuffer)->numChannels);
    }
    mpOutput->mMediaFormat.isSampleInterleave = mpInput->mMediaFormat.isSampleInterleave = mpElementListHead->outputFormat.isSampleInterleave = 1 ;
    mpOutput->mMediaFormat.auChannels = mpInput->mMediaFormat.auChannels = mpElementListHead->outputFormat.nChannels = ((CAudioBuffer*)mpInBuffer)->numChannels;
    mpOutput->mMediaFormat.auSampleFormat = mpInput->mMediaFormat.auSampleFormat = mpElementListHead->outputFormat.sampleFormat = ((CAudioBuffer*)mpInBuffer)->sampleFormat;
    mpOutput->mMediaFormat.auSamplerate = mpInput->mMediaFormat.auSamplerate = mpElementListHead->outputFormat.sampleRate = ((CAudioBuffer*)mpInBuffer)->sampleRate;

    //output
    //android current restrict
    if (mpInput->mMediaFormat.auChannels > 2) {
        AMLOG_INFO("input audio's channels %d greater than 2.\n", mpInput->mMediaFormat.auChannels);
        mpOutput->mMediaFormat.auChannels = 2;
    }
    if ((mpInput->mMediaFormat.auSampleFormat!=SAMPLE_FMT_S16) && (mpInput->mMediaFormat.auSampleFormat!=SAMPLE_FMT_U8)) {
        AMLOG_INFO("input audio's sample format %d need convert.\n", mpInput->mMediaFormat.auSampleFormat);
        mpOutput->mMediaFormat.auSampleFormat = SAMPLE_FMT_S16;
    }
    if (mpInput->mMediaFormat.auSamplerate > 48000) {
        AMLOG_INFO("input audio's sample format %d need convert.\n", mpInput->mMediaFormat.auSampleFormat);
        mpOutput->mMediaFormat.auSamplerate = 48000;
    }

    mpElementListHead->inputFormat.isChannelInterleave = mpOutput->mMediaFormat.isChannelInterleave;
    mpElementListHead->inputFormat.isSampleInterleave = mpOutput->mMediaFormat.isSampleInterleave;
    mpElementListHead->inputFormat.nChannels= mpOutput->mMediaFormat.auChannels;
    mpElementListHead->inputFormat.sampleFormat = mpOutput->mMediaFormat.auSampleFormat;
    mpElementListHead->inputFormat.sampleRate = mpOutput->mMediaFormat.auSamplerate;

    AMLOG_INFO("UpdateFormat: input isChannelInterleave %d, isSampleInterleave %d, auChannels %d, auSampleFormat %d, auSamplerate %d.\n", mpInput->mMediaFormat.isChannelInterleave, mpInput->mMediaFormat.isSampleInterleave, mpInput->mMediaFormat.auChannels, mpInput->mMediaFormat.auSampleFormat, mpInput->mMediaFormat.auSamplerate);
    AMLOG_INFO("UpdateFormat: output isChannelInterleave %d, isSampleInterleave %d, auChannels %d, auSampleFormat %d, auSamplerate %d.\n", mpOutput->mMediaFormat.isChannelInterleave, mpOutput->mMediaFormat.isSampleInterleave, mpOutput->mMediaFormat.auChannels, mpOutput->mMediaFormat.auSampleFormat, mpOutput->mMediaFormat.auSamplerate);

}

bool CAudioEffecter::ProcessCmd(CMD& cmd)
{
    AMLOG_CMD("****CAudioEffecter::ProcessCmd, cmd.code %d, state %d.\n", cmd.code, msState);
    switch (cmd.code) {
        case CMD_STOP:
            mbRun = false;
            AMLOG_DEBUG("****CAudioEffecter::ProcessCmd, STOP cmd.\n");
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
            if (mpInBuffer) {
                mpInBuffer->Release();
                mpInBuffer = NULL;
            }
            CmdAck(ME_OK);
            break;

        case CMD_AVSYNC:
            CmdAck(ME_OK);
            break;

        case CMD_BEGIN_PLAYBACK:
            AM_ASSERT(msState == STATE_PENDING);
            AM_ASSERT(!mpInBuffer);
            msState = STATE_IDLE;
            break;

        case CMD_SOURCE_FILTER_BLOCKED:
            break;

        default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }
    return false;
}

void CAudioEffecter::ProcessData()
{
    CBuffer* pOutbuffer = NULL;
    AM_ASSERT(msState == STATE_READY);
    AM_ASSERT(mpInBuffer);
    CSharedAudioBuffer* pSharedBuffer = (CSharedAudioBuffer*)mpInBuffer;//input buffer
    CAudioBuffer* pCarrier = pSharedBuffer->mpCarrier;//output buffer
    AM_ASSERT(pSharedBuffer->mbAttached);
    AM_ASSERT(pCarrier);
    AM_ASSERT(pCarrier->GetDataPtr() == mpInBuffer->GetDataPtr());

    //send buffer
    pOutbuffer = (CBuffer*) pCarrier;

    if (!mbSkip) {

        AMLOG_DEBUG("mpInBuffer %p, size %d.\n", mpInBuffer->GetDataPtr(), mpInBuffer->GetDataSize());
#ifdef __use_hardware_timmer__
        AM_UINT time = AM_get_hw_timer_count();
        PrepareInOutBuffer(pOutbuffer);
        mCurrentInSamples = ExecutePipeline(mCurrentInSamples);
        totalTime += AM_hwtimer2us(AM_get_hw_timer_count() - time);
#else
        struct timeval tvbefore, tvafter;
        gettimeofday(&tvbefore,NULL);
        PrepareInOutBuffer(pOutbuffer);
        mCurrentInSamples = ExecutePipeline(mCurrentInSamples);
        gettimeofday(&tvafter,NULL);
        totalTime += (tvafter.tv_sec -tvbefore.tv_sec)*1000000 + tvafter.tv_usec - tvbefore.tv_usec;
#endif
        AM_ASSERT(mCurrentInSamples <= mMaxInSamples);
        AMLOG_DEBUG("output size %d, nsamples %d.\n", mpOutput->mMediaFormat.auChannels * _getSize(mpOutput->mMediaFormat.auSampleFormat) * mCurrentInSamples, mCurrentInSamples);
        pOutbuffer->SetDataSize(mpOutput->mMediaFormat.auChannels * _getSize(mpOutput->mMediaFormat.auSampleFormat) * mCurrentInSamples);
    }else {
        pOutbuffer->SetDataSize(mpInBuffer->GetDataSize());
    }

    pOutbuffer->SetType(CBuffer::DATA);
    mPTS = pOutbuffer->mPTS = mpInBuffer->mPTS;
    //fill changed format
    pCarrier->sampleRate = mpOutput->mMediaFormat.auSamplerate;
    pCarrier->numChannels = mpOutput->mMediaFormat.auChannels;
    pCarrier->sampleFormat = mpOutput->mMediaFormat.auSampleFormat;

    pSharedBuffer->mbAttached = false;
    mpOutput->SendBuffer(pOutbuffer);
    mpInBuffer = NULL;
    decodedFrame ++;
}

void CAudioEffecter::ProcessRemainingDataOnly()
{
    CBuffer* pOutbuffer = NULL;
    AM_ASSERT(msState == STATE_READY);
    AM_ASSERT(!mpInBuffer);
    AM_ASSERT(!mbSkip);
    AM_ASSERT(mpCurrentPtr);
    AM_ASSERT(mCurrentDataSize);
    AM_ASSERT(mpBufferPool->GetFreeBufferCnt());
    if (!mpOutput->AllocBuffer(pOutbuffer)) {
        msState = STATE_PENDING;
        AMLOG_ERROR("CAudioEffecter::ProcessRemainingDataOnly: allocate fail.\n");
        return;
    }

    AMLOG_INFO("remaining data %p, size %d, free buffer count %d.\n", mpCurrentPtr, mCurrentDataSize, mpBufferPool->GetFreeBufferCnt());
#ifdef __use_hardware_timmer__
    AM_UINT time = AM_get_hw_timer_count();
    PrepareInOutBuffer(pOutbuffer);
    mCurrentInSamples = ExecutePipeline(mCurrentInSamples);
    totalTime += AM_hwtimer2us(AM_get_hw_timer_count() - time);
#else
    struct timeval tvbefore, tvafter;
    gettimeofday(&tvbefore,NULL);
    PrepareInOutBuffer(pOutbuffer);
    mCurrentInSamples = ExecutePipeline(mCurrentInSamples);
    gettimeofday(&tvafter,NULL);
    totalTime += (tvafter.tv_sec -tvbefore.tv_sec)*1000000 + tvafter.tv_usec - tvbefore.tv_usec;
#endif
    AM_ASSERT(mCurrentInSamples <= mMaxInSamples);
    pOutbuffer->SetDataSize(mpOutput->mMediaFormat.auChannels * _getSize(mpOutput->mMediaFormat.auSampleFormat) * mCurrentInSamples);

    pOutbuffer->SetType(CBuffer::DATA);
    pOutbuffer->mPTS = mPTS;
    //fill changed format
    ((CAudioBuffer*)pOutbuffer)->sampleRate = mpOutput->mMediaFormat.auSamplerate;
    ((CAudioBuffer*)pOutbuffer)->numChannels = mpOutput->mMediaFormat.auChannels;
    ((CAudioBuffer*)pOutbuffer)->sampleFormat = mpOutput->mMediaFormat.auSampleFormat;

    mpOutput->SendBuffer(pOutbuffer);
    decodedFrame ++;
}

void CAudioEffecter::OnRun()
{
    CmdAck(ME_OK);

#ifdef __use_hardware_timmer__
    AM_open_hw_timer();
#endif
    AM_ERR err;
    totalTime = 0;
    decodedFrame = 0;
    droppedFrame = 0;
    mpBufferPool = mpOutput->mpBufferPool;
    mpAttachedBufferPool->AttachToBufferPool(mpBufferPool);

    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
    mbRun = true;
    msState = STATE_IDLE;
    bool start = false;

    AMLOG_INFO("CAudioEffecter %p start OnRun loop.\n", this);

    while (mbRun) {
        AMLOG_STATE("start switch, msState=%d, %d input data, %d free buffers.\n", msState, mpInput->mpBufferQ->GetDataCnt(), mpBufferPool->GetFreeBufferCnt());

        switch (msState) {

            case STATE_IDLE:
                if (mbPaused) {
                    msState = STATE_PENDING;
                    break;
                }

                if (mpBufferPool->GetFreeBufferCnt() > mReservedBuffers) {
                    msState = STATE_HAS_OUTPUTBUFFER;
                    break;
                }

                if (mCurrentDataSize) {
                    //has remaining data, should not come here on current version, add in future
                    AM_ASSERT(0);
                    if (mpBufferPool->GetFreeBufferCnt() > 0) {
                        msState = STATE_READY;
                    } else {
                        msState = STATE_HAS_INPUTDATA;
                    }
                    break;
                }

                type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
                if(type == CQueue::Q_MSG) {
                    ProcessCmd(cmd);
                } else {
                    AM_ASSERT((CQueueInputPin*)result.pOwner == (CQueueInputPin*)mpInput);
                    if (ReadInputData()) {
                        msState = STATE_READY;
                    }
                }
                break;

            case STATE_HAS_OUTPUTBUFFER:
                ConveyBuffers();
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
                AM_ASSERT(mpInBuffer);

                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case STATE_READY:
                if (mCurrentDataSize) {
                    ProcessRemainingDataOnly();
                } else if (mpInBuffer) {
                    //has input buffer
                    if (!start || ((CAudioBuffer*)mpInBuffer)->sampleFormat != mpInput->mMediaFormat.auSampleFormat \
                                  || ((CAudioBuffer*)mpInBuffer)->numChannels != mpInput->mMediaFormat.auChannels \
                                  || ((CAudioBuffer*)mpInBuffer)->sampleRate != mpInput->mMediaFormat.auSamplerate ) {
                        AMLOG_INFO("format updated....\n");
                        AMLOG_INFO("Detailed info, new input: sampleFormat %d, numChannels %d, sampleRate %d.\n", ((CAudioBuffer*)mpInBuffer)->sampleFormat, ((CAudioBuffer*)mpInBuffer)->numChannels, ((CAudioBuffer*)mpInBuffer)->sampleRate);
                        AMLOG_INFO("Detailed info, original input: sampleFormat %d, numChannels %d, sampleRate %d.\n", mpInput->mMediaFormat.auSampleFormat, mpInput->mMediaFormat.auChannels, mpInput->mMediaFormat.auSamplerate);
                        start = true;
                        ClearPipeline();
                        UpdateFormat();
                        err = ConfigPipeline(&mpInput->mMediaFormat, &mpOutput->mMediaFormat);
                        if (err!= ME_OK) {
                            AMLOG_INFO("ConfigPipeline fail.\n");
                            mbSkip = true;
                        } else {
                            err = SetupPipelineBuffers();
                            if (err!= ME_OK) {
                                AMLOG_INFO("SetupPipelineBuffers fail.\n");
                                mbSkip = true;
                            }
                        }
                    }
                    if (mpInBuffer->GetDataSize() && mpInBuffer->GetDataPtr()) {
                        ProcessData();
                    } else {
                        AMLOG_WARN("NULL packet here, mpInBuffer->GetDataPtr() %p, mpInBuffer->GetDataSize() %d.\n", mpInBuffer->GetDataPtr(), mpInBuffer->GetDataSize());
                        msState = STATE_IDLE;
                        mpInBuffer->Release();
                        mpInBuffer = NULL;
                        break;
                    }
                } else {
                    AMLOG_ERROR("no data?\n");
                    msState = STATE_IDLE;
                    break;
                }

                if (!mCurrentDataSize && !mpInBuffer) {
                    msState = STATE_IDLE;
                } else if (!mpBufferPool->GetFreeBufferCnt()) {
                    //wait for output buffer
                    msState = STATE_HAS_INPUTDATA;
                } else {
                    //process msg first
                    CMD cmd;
                    if(mpWorkQ->PeekCmd(cmd)) {
                        ProcessCmd(cmd);
                    }
                }
                break;

            default:
                AM_ERROR(" %d",(AM_UINT)msState);
                break;
        }

        AMLOG_STATE("CAudioEffecter %p end switch, msState=%d, mbRun = %d.\n", this, msState, mbRun);
    }

    if(mpInBuffer) {
        mpInBuffer->Release();
        mpInBuffer = NULL;
    }

    AMLOG_INFO("CAudioEffecter %p OnRun exit, msState=%d.\n", this, msState);
}

bool CAudioEffecter::SendEOS(CAudioEffecterOutput *pPin)
{
    CBuffer *pOutbuffer;
    CSharedAudioBuffer* pSharedBuffer = (CSharedAudioBuffer*)mpInBuffer;//input buffer
    CAudioBuffer* pCarrier = pSharedBuffer->mpCarrier;//output buffer
    AM_ASSERT(pSharedBuffer->mbAttached);
    AM_ASSERT(pCarrier);
    AM_ASSERT(pCarrier->GetDataPtr() == mpInBuffer->GetDataPtr());

    AM_ASSERT(pSharedBuffer->mbAttached == true);
    pSharedBuffer->mbAttached = false;

    //send buffer
    pOutbuffer = (CBuffer*) pCarrier;
    pOutbuffer->SetType(CBuffer::EOS);
    AMLOG_INFO("CAudioEffecter Send EOS\n");

    pPin->SendBuffer(pOutbuffer);
    mpInBuffer = NULL;

    return true;
}

AM_ERR CAudioEffecter::SetInputFormat()
{
    mpOutput->mMediaFormat.pMediaType = &GUID_Decoded_Audio;
    mpOutput->mMediaFormat.pFormatType = &GUID_Audio_PCM;
    mpOutput->mMediaFormat.auChannels = mpInput->mMediaFormat.auChannels;
    mpOutput->mMediaFormat.auSampleFormat = mpInput->mMediaFormat.auSampleFormat;
    mpOutput->mMediaFormat.auSamplerate = mpInput->mMediaFormat.auSamplerate;
    mpOutput->mMediaFormat.isChannelInterleave = mpInput->mMediaFormat.isChannelInterleave;
    AMLOG_INFO("SetInputFormat isChannelInterleave %d, auChannels %d, auSampleFormat %d, auSamplerate %d.\n", mpInput->mMediaFormat.isChannelInterleave, mpInput->mMediaFormat.auChannels, mpInput->mMediaFormat.auSampleFormat, mpInput->mMediaFormat.auSamplerate);

    //android current restrict
    if (mpInput->mMediaFormat.auChannels > 2) {
        AMLOG_INFO("input audio's channels %d greater than 2.\n", mpInput->mMediaFormat.auChannels);
        mpOutput->mMediaFormat.auChannels = 2;
    }
    if ((mpInput->mMediaFormat.auSampleFormat!=SAMPLE_FMT_S16) && (mpInput->mMediaFormat.auSampleFormat!=SAMPLE_FMT_U8)) {
        AMLOG_INFO("input audio's sample format %d need convert.\n", mpInput->mMediaFormat.auSampleFormat);
        mpOutput->mMediaFormat.auSampleFormat = SAMPLE_FMT_S16;
    }

#if PLATFORM_ANDROID
    if (mpInput->mMediaFormat.auSamplerate > 48000) {
        AMLOG_INFO("input audio's sample format %d need convert.\n", mpInput->mMediaFormat.auSampleFormat);
        mpOutput->mMediaFormat.auSamplerate = 48000;
    }
#else
    //tmp modify here, for linux audio freq/clock issue
    if (mpInput->mMediaFormat.auSamplerate != 44100) {
        AMLOG_INFO("input audio's sample format %d need convert.\n", mpInput->mMediaFormat.auSampleFormat);
        mpOutput->mMediaFormat.auSamplerate = 44100;
    }
#endif

    AM_ASSERT(mpOutput->mMediaFormat.isChannelInterleave == 0);

    return ME_OK;
}

CEffectElement* CAudioEffecter::ElementFactory(EAudioEffectType type, AM_UINT& index)
{
    CEffectElement* pEffect = NULL;
    switch (type) {
        case eFormatConvertor:
            if (index == 0) {
                pEffect = new CFormatConvertor();
                index = 1;
            } else {
                //to do add others
            }
            break;
        case eDeinterleaver:
            if (index == 0) {
                pEffect = new CDeInterleaver();
                index = 1;
            } else {
                //to do add others
            }
            break;
        case eDeinterleaverDownmixer:
            if (index == 0) {
                pEffect = new CDeInterleaverDownMixer();
                index = 1;
            } else {
                //to do add others
            }
            break;
        case eInterleaver:
            if (index == 0) {
                pEffect = new CInterleaver();
                index = 1;
            } else {
                //to do add others
            }
            break;
        case eDownMixer:
            if (index == 0) {
                pEffect = new CDownMixer();
                index = 1;
            }else if (index == 1) {
                pEffect = new CSimpleDownMixer();
                index = 2;
            } else {
                //to do add others
            }
            break;
         case eSampleRateConvertor:
            if (index == 0) {
                pEffect = new CSampleRateConvertor();
                index = 1;
            } else {
                //to do add others
            }
            break;
         default:
            AMLOG_ERROR("wrong EAudioEffectType %d.\n", type);
            break;
    }
    return pEffect;
}

AM_ERR CAudioEffecter::SetupElement(SEffectElementList* pNode)
{
    AM_UINT index = 0;
    bool success = false;
    AM_ASSERT(!pNode->pElement);

    do {

        //create
        pNode->pElement = ElementFactory(pNode->type, index);
        if (!pNode->pElement) {
            break;
        }

        //config
        success = pNode->pElement->ConfigInputOutputFormat(&pNode->inputFormat, &pNode->outputFormat);
        if (success) {
            AMLOG_INFO("setup element(%s) %d success, index %d.\n", pNode->pElement->GetName(), pNode->type, index);
            AMLOG_INFO("input format: isChannelInterleave %d, nChannels %d, sampleFormat %d, sampleRate %d.\n", pNode->inputFormat.isChannelInterleave, pNode->inputFormat.nChannels, pNode->inputFormat.sampleFormat, pNode->inputFormat.sampleRate);
            AMLOG_INFO("output format: isChannelInterleave %d, nChannels %d, sampleFormat %d, sampleRate %d.\n", pNode->outputFormat.isChannelInterleave, pNode->outputFormat.nChannels, pNode->outputFormat.sampleFormat, pNode->outputFormat.sampleRate);
            return ME_OK;
        }

        //delete
        AM_ASSERT(pNode->pElement);
        delete pNode->pElement;
        pNode->pElement = NULL;
    } while (1);

    AMLOG_ERROR("setup element %d fail, index %d.\n", (AM_UINT)pNode->type, index);
    return ME_ERROR;
}

SEffectElementList* CAudioEffecter::AddNewNode(SEffectElementList* list)
{
    SEffectElementList* tail = new SEffectElementList();

    if (list) {
        //attach to tail
        tail->pPre = list;
        tail->pNext = list->pNext;
        list->pNext->pPre = tail;
        list->pNext = tail;
    } else {
        //new head
        tail->pPre = tail->pNext = tail;
    }
    return tail;
}

AM_UINT CAudioEffecter::ExecutePipeline(AM_UINT nSamples)
{
    SEffectElementList* pEl = mpElementListHead->pNext;
    while (pEl != mpElementListHead) {
        AM_ASSERT(pEl->pElement);
        if (pEl->pElement) {
            AMLOG_VERBOSE("%s, nsampls %d, in %p, out %p, inmaxsize %d, outmaxsize %d.\n", pEl->pElement->GetName(), nSamples, pEl->inputBuffer.pBuffer[0], pEl->pNext->inputBuffer.pBuffer[0], pEl->inputBuffer.bufferSize, pEl->pNext->inputBuffer.bufferSize);
            nSamples = pEl->pElement->Process(&pEl->inputBuffer, &pEl->pNext->inputBuffer, nSamples);
            AMLOG_VERBOSE("done samples %d.\n", nSamples);
        }
        pEl = pEl->pNext;
    }
    return nSamples;
}


AM_ERR CAudioEffecter::ConfigPipeline(CAudioMediaFormat* input, CAudioMediaFormat* output)
{
    AM_INT target_format = input->auSampleFormat;
    AMLOG_INFO("ConfigPipeline mEffectFlag %d.\n", mEffectFlag);
    AM_ERR err = ME_OK;

    AMLOG_INFO("ConfigPipeline: input isChannelInterleave %d, auChannels %d, auSampleFormat %d, auSamplerate %d.\n", mpInput->mMediaFormat.isChannelInterleave, mpInput->mMediaFormat.auChannels, mpInput->mMediaFormat.auSampleFormat, mpInput->mMediaFormat.auSamplerate);
    AMLOG_INFO("ConfigPipeline: output isChannelInterleave %d, auChannels %d, auSampleFormat %d, auSamplerate %d.\n", mpOutput->mMediaFormat.isChannelInterleave, mpOutput->mMediaFormat.auChannels, mpOutput->mMediaFormat.auSampleFormat, mpOutput->mMediaFormat.auSamplerate);

    mEffectFlag = 0; //reset flag

    if (input->auChannels != output->auChannels) {
        AMLOG_INFO("channel number(in %d, out %d) not match, need DownMixer.\n", input->auChannels, output->auChannels);
        mEffectFlag |= DEffect_DownMixer;
        mbSkip = false;
    }
    if (input->auSamplerate != output->auSamplerate) {
        AMLOG_INFO("input audio's sample rate (in %d, out %d), need convert.\n", input->auSamplerate, output->auSamplerate);
        mEffectFlag |= DEffect_SampleRateConvertor;
        mbSkip = false;
    }
    if (mEffectFlag&(~DEffect_DownMixer)) {
        target_format = SAMPLE_FMT_S16;
    }

    AMLOG_DEBUG("mEffectFlag 0x%x.\n", mEffectFlag);

    //if have effects, it should change input to channel interlave
    if ((mEffectFlag&(~DEffect_DownMixer)) && !mpElementListHead->outputFormat.isChannelInterleave) {
        AMLOG_INFO("have effects, need change to channel interlave.\n");
        AM_ASSERT(output->auChannels != 1);
        AM_ASSERT(input->auChannels != 1);
        AM_ASSERT(mpElementListHead->outputFormat.isSampleInterleave);
        AM_ASSERT(mpElementListHead->pPre == mpElementListTail);
        mpElementListTail = AddNewNode(mpElementListTail);

        mbSkip = false;
        mpElementListTail->outputFormat = mpElementListTail->inputFormat = mpElementListTail->pPre->outputFormat;
        mpElementListTail->outputFormat.isChannelInterleave = 1;
        mpElementListTail->outputFormat.isSampleInterleave = 0;
        //if have to downmixer, try use deinterleaverdownmixer
        if (output->auChannels != input->auChannels) {
            mbDeInterleaverDownmixer = true;
            mEffectFlag &= ~DEffect_DownMixer;
            mpElementListTail->type = eDeinterleaverDownmixer;
            mpElementListTail->outputFormat.nChannels = mpElementListHead->inputFormat.nChannels;
        } else {
            mpElementListTail->type = eDeinterleaver;
        }
        if ((err = SetupElement(mpElementListTail)) != ME_OK) {
            return err;
        }
    }

    //format convertor
    if (mpElementListTail->outputFormat.sampleFormat != target_format) {
        AMLOG_INFO("sample format(in %d out %d) not match, need convert.\n", mpElementListTail->outputFormat.sampleFormat, target_format);
        AM_ASSERT(mpElementListHead->pPre == mpElementListTail);
        mpElementListTail = AddNewNode(mpElementListTail);

        mbSkip = false;
        mpElementListTail->type = eFormatConvertor;
        mpElementListTail->inputFormat = mpElementListTail->outputFormat = mpElementListTail->pPre->outputFormat;
        mpElementListTail->outputFormat.sampleFormat = target_format;
        if ((err = SetupElement(mpElementListTail)) != ME_OK) {
            return err;
        }
    }

    //add effects here
    if (mEffectFlag & DEffect_DownMixer) {
        AMLOG_INFO("need downmix, in channels %d out channels %d.\n", mpElementListTail->outputFormat.nChannels, mpElementListHead->inputFormat.nChannels);
        AM_ASSERT(mpElementListHead->pPre == mpElementListTail);
        mpElementListTail = AddNewNode(mpElementListTail);

        mbSkip = false;
        mpElementListTail->type = eDownMixer;
        mpElementListTail->outputFormat = mpElementListTail->inputFormat = mpElementListTail->pPre->outputFormat;
        mpElementListTail->outputFormat.nChannels = mpElementListHead->inputFormat.nChannels;//target nChannels
        if ((err = SetupElement(mpElementListTail)) != ME_OK) {
            return err;
        }
        mnEffects ++;
    }

    //sample rate convertor
    if (mEffectFlag & DEffect_SampleRateConvertor) {
        AMLOG_INFO("need sample rate convertor, sample rate in %d out %d.\n", mpElementListTail->outputFormat.sampleRate, mpElementListHead->inputFormat.sampleRate);
        AM_ASSERT(mpElementListHead->pPre == mpElementListTail);
        mpElementListTail = AddNewNode(mpElementListTail);

        mbSkip = false;
        mpElementListTail->type = eSampleRateConvertor;
        mpElementListTail->outputFormat = mpElementListTail->inputFormat = mpElementListTail->pPre->outputFormat;
        mpElementListTail->outputFormat.sampleRate= mpElementListHead->inputFormat.sampleRate;//target nChannels
        if ((err = SetupElement(mpElementListTail)) != ME_OK) {
            return err;
        }
        mnEffects ++;

    }

    if (mnEffects) {
        //mark effect addin entry point
        mpAddEffectEntryPoint = mpElementListTail;
    }

    //convert back
    //format convertor
    if (mpElementListTail->outputFormat.sampleFormat != mpElementListHead->inputFormat.sampleFormat) {
        AMLOG_INFO("sample format(in %d out %d) not match, need convert.\n", mpElementListTail->outputFormat.sampleFormat, mpElementListHead->inputFormat.sampleFormat);
        AM_ASSERT(mpElementListHead->pPre == mpElementListTail);
        mpElementListTail = AddNewNode(mpElementListTail);

        mbSkip = false;
        mpElementListTail->type = eFormatConvertor;
        mpElementListTail->inputFormat = mpElementListTail->outputFormat = mpElementListTail->pPre->outputFormat;
        mpElementListTail->outputFormat.sampleFormat = mpElementListHead->inputFormat.sampleFormat;
        if ((err = SetupElement(mpElementListTail)) != ME_OK) {
            return err;
        }
    }

    //interleaver
    if (!mpElementListTail->outputFormat.isSampleInterleave && mpElementListHead->inputFormat.isSampleInterleave) {
        AM_ASSERT(mpElementListTail->outputFormat.isChannelInterleave);
        AM_ASSERT(!mpElementListHead->inputFormat.isChannelInterleave);
        AM_ASSERT(!mpElementListTail->outputFormat.isSampleInterleave);
        AM_ASSERT(mpElementListHead->inputFormat.isSampleInterleave);
        AM_ASSERT(mpElementListHead->pPre == mpElementListTail);
        AMLOG_INFO("not sample interleave in channal interleave%d, out channel interleave %d, need interleaver.\n", mpElementListHead->inputFormat.isChannelInterleave, mpElementListHead->inputFormat.isSampleInterleave);
        mpElementListTail = AddNewNode(mpElementListTail);

        mbSkip = false;
        mpElementListTail->type = eInterleaver;
        mpElementListTail->outputFormat = mpElementListTail->inputFormat = mpElementListTail->pPre->outputFormat;
        mpElementListTail->outputFormat.isChannelInterleave = 0;
        mpElementListTail->outputFormat.isSampleInterleave = 1;
        if ((err = SetupElement(mpElementListTail)) != ME_OK) {
            return err;
        }
    }

    //calculate attached buffer ratio
    mBufferSizeM = _getSize(input->auSampleFormat)*input->auChannels*input->auSamplerate/100;
    mBufferSizeN = _getSize(output->auSampleFormat)*output->auChannels*output->auSamplerate/100;
    mMaxOutSamples = AVCODEC_MAX_AUDIO_FRAME_SIZE/_getSize(output->auSampleFormat)/output->auChannels;
    mMaxInSamples = mMaxOutSamples*(input->auSamplerate/100)/(output->auSamplerate/100);
    AM_UINT memory_retrict = AVCODEC_MAX_AUDIO_FRAME_SIZE/_getSize(input->auSampleFormat)/input->auChannels;
    mMaxInSamples = (memory_retrict > mMaxInSamples)? mMaxInSamples: memory_retrict;
    if (mBufferSizeM < mBufferSizeN) {
        //should allocate reserved buffer
        mpReservedBufferBase = (AM_U8*)malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE);
    }
    AMLOG_DEBUG("**** mBufferSizeM %d, mBufferSizeN %d, mMaxInSamples %d, mMaxOutSamples %d.\n", mBufferSizeM, mBufferSizeN, mMaxInSamples, mMaxOutSamples);
    AMLOG_DEBUG("ConfigPipeline done: input isChannelInterleave %d, auChannels %d, auSampleFormat %d, auSamplerate %d.\n", mpInput->mMediaFormat.isChannelInterleave, mpInput->mMediaFormat.auChannels, mpInput->mMediaFormat.auSampleFormat, mpInput->mMediaFormat.auSamplerate);
    AMLOG_DEBUG("ConfigPipeline done: output isChannelInterleave %d, auChannels %d, auSampleFormat %d, auSamplerate %d.\n", mpOutput->mMediaFormat.isChannelInterleave, mpOutput->mMediaFormat.auChannels, mpOutput->mMediaFormat.auSampleFormat, mpOutput->mMediaFormat.auSamplerate);
    AMLOG_DEBUG("ConfigPipeline done: input isChannelInterleave %d, auChannels %d, auSampleFormat %d, auSamplerate %d.\n", input->isChannelInterleave, input->auChannels, input->auSampleFormat, input->auSamplerate);
    AMLOG_DEBUG("ConfigPipeline done: output isChannelInterleave %d, auChannels %d, auSampleFormat %d, auSamplerate %d.\n", output->isChannelInterleave, output->auChannels, output->auSampleFormat, output->auSamplerate);
    return ME_OK;
}

AM_ERR CAudioEffecter::SetupPipelineBuffers()
{
    AMLOG_INFO("start SetupPipelineBuffers.\n");
    if (!mpElementListHead || !mpElementListTail) {
        AMLOG_ERROR("configed fail.\n");
        return ME_ERROR;
    }

    SEffectElementList* pEl = mpElementListHead->pNext;
    AM_ASSERT(mpElementListHead->pPre == mpElementListTail);
    AM_ASSERT(mpElementListTail->pNext == mpElementListHead);
    AM_ASSERT(mMaxOutSamples);
    AM_ASSERT(mMaxInSamples);
    AM_UINT maxSamples = (mMaxOutSamples > mMaxInSamples)? mMaxOutSamples:mMaxInSamples;

    AM_UINT samplesize, channels, isChannelInterleave, isSampleInterleave;

#if 0
    //check if first element is sample rate convertor, allocate input buffer
    if (pEl->type == eSampleRateConvertor) {
        mbPipelineInputBufferAllocated = true;
        pEl->pElement->QueryBufferInfo(samplesize, channels, isChannelInterleave, true);
        pEl->bInputAllocated = 1;
        if (isChannelInterleave) {
            pEl->inputBuffer.bufferNumber = channels;
            pEl->inputBuffer.bufferSize = samplesize*mMaxOutSamples + DSampleConvertorReservedMemorySize;
            for (AM_UINT i = 0; i < channels; i++) {
                pEl->inputBuffer.pBufferBase[i] = pEl->inputBuffer.pBuffer[i] = (AM_U8*) malloc(pEl->inputBuffer.bufferSize);
            }
        } else {
            pEl->inputBuffer.bufferNumber = 1;
            pEl->inputBuffer.bufferSize = (samplesize*mMaxOutSamples + DSampleConvertorReservedMemorySize) * channels;
            pEl->inputBuffer.pBufferBase[0] = pEl->inputBuffer.pBuffer[0] = (AM_U8*) malloc(pEl->inputBuffer.bufferSize);
        }
    } else {
        //first node use input buffer
        pEl->bInputAllocated = 0;
    }
#endif

    while (pEl != mpElementListHead) {
        AM_ASSERT(pEl->pElement);

        //setup output buffers
        pEl->pElement->QueryBufferInfo(samplesize, channels, isChannelInterleave, isSampleInterleave, true);

        if ((pEl->pPre!=mpElementListHead && !pEl->pPre->pElement->CanShareBuffer()) || pEl->type == eSampleRateConvertor) {
            //sample rate convertor need reserved memory(pEl->pNext->input/pEl->output), handled here
            pEl->bInputAllocated = 1;
            if(pEl->pPre == mpElementListHead)
                mbPipelineInputBufferAllocated = true;
            if (isChannelInterleave) {
                pEl->inputBuffer.bufferNumber = channels;
                pEl->inputBuffer.bufferSize =(pEl->pNext->type != eSampleRateConvertor)? samplesize*maxSamples : samplesize*maxSamples + DSampleConvertorReservedMemorySize;
                for (AM_UINT i = 0; i < channels; i++) {
                    pEl->inputBuffer.pBufferBase[i] = pEl->inputBuffer.pBuffer[i] = (AM_U8*) malloc(pEl->inputBuffer.bufferSize);
                }
            } else if (isSampleInterleave) {
                pEl->inputBuffer.bufferNumber = 1;
                pEl->inputBuffer.bufferSize = (samplesize*maxSamples + ((pEl->type != eSampleRateConvertor)? 0 : DSampleConvertorReservedMemorySize)) * channels;
                pEl->inputBuffer.pBufferBase[0] = pEl->inputBuffer.pBuffer[0] = (AM_U8*) malloc(pEl->inputBuffer.bufferSize);
            }
        }
        pEl = pEl->pNext;

    }

    AMLOG_INFO("SetupPipelineBuffers done.\n");
    return ME_OK;
}

void CAudioEffecter::ClearPipeline()
{
    if (!mpElementListHead) {
        AMLOG_WARN("pipeline has been cleared before .\n");
        return;
    }
    AM_UINT i = 0;
    SEffectElementList* pEl = mpElementListHead->pNext;

    while (pEl != mpElementListHead) {

        //clear buffers
        if (pEl->bInputAllocated) {
            for (i = 0; i < pEl->inputBuffer.bufferNumber; i++) {
                AM_ASSERT(pEl->inputBuffer.pBufferBase[i]);
                if (pEl->inputBuffer.pBufferBase[i])
                    free(pEl->inputBuffer.pBufferBase[i]);
                pEl->inputBuffer.pBufferBase[i] = NULL;
            }
        }

        //clear pipeline
        AM_ASSERT(pEl->pElement);
        if (pEl->pElement) {
            delete pEl->pElement;
            pEl->pElement = NULL;
        }

        pEl = pEl->pNext;
        delete pEl->pPre;
    }

    delete mpElementListHead;
    mpElementListHead = NULL;
    mpElementListTail = NULL;
    mpAddEffectEntryPoint = NULL;
    if (mpReservedBufferBase) {
        free(mpReservedBufferBase);
        mpReservedBufferBase = NULL;
    }
    return;
}

#ifdef AM_DEBUG
void CAudioEffecter::PrintState()
{
    AMLOG_INFO("CAudioEffecter: msState=%d, %d input data, %d free buffers mbSkip %d.\n", msState, mpInput->mpBufferQ->GetDataCnt(), mpBufferPool->GetFreeBufferCnt(), mbSkip);
}
#endif

//-----------------------------------------------------------------------
//
// CAudioEffecterInput
//
//-----------------------------------------------------------------------
CAudioEffecterInput *CAudioEffecterInput::Create(CFilter *pFilter)
{
    CAudioEffecterInput *result = new CAudioEffecterInput(pFilter);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CAudioEffecterInput::Construct()
{
    AM_ERR err = inherited::Construct(((CAudioEffecter*)mpFilter)->MsgQ());
    if (err != ME_OK)
        return err;
    return ME_OK;
}

CAudioEffecterInput::~CAudioEffecterInput()
{
    AMLOG_DESTRUCTOR("~CAudioEffecterInput.\n");
}

AM_ERR CAudioEffecterInput::CheckMediaFormat(CMediaFormat *pFormat)
{
    if (*pFormat->pFormatType == GUID_Format_FFMPEG_Media) {
        //AM_INFO("samplerate:%d,channels:%d\n",((CFFMpegMediaFormat *)pFormat)->auSamplerate, ((CFFMpegMediaFormat *)pFormat)->auChannels);
        mMediaFormat.auSampleFormat = ((CFFMpegMediaFormat *)pFormat)->auSampleFormat;
        mMediaFormat.auSamplerate = ((CFFMpegMediaFormat *)pFormat)->auSamplerate;
        mMediaFormat.auChannels = ((CFFMpegMediaFormat *)pFormat)->auChannels;
        mMediaFormat.isChannelInterleave = ((CFFMpegMediaFormat *)pFormat)->isChannelInterleave;
    } else if (*pFormat->pFormatType == GUID_Audio_PCM) {
        mMediaFormat.auSampleFormat = ((CAudioMediaFormat *)pFormat)->auSampleFormat;
        mMediaFormat.auSamplerate = ((CAudioMediaFormat *)pFormat)->auSamplerate;
        mMediaFormat.auChannels = ((CAudioMediaFormat *)pFormat)->auChannels;
        mMediaFormat.isChannelInterleave = ((CAudioMediaFormat *)pFormat)->isChannelInterleave;
    }
    return ((CAudioEffecter*)mpFilter)->SetInputFormat();
}

//-----------------------------------------------------------------------
//
// CAudioEffecterOutput
//
//-----------------------------------------------------------------------
CAudioEffecterOutput *CAudioEffecterOutput::Create(CFilter *pFilter)
{
    CAudioEffecterOutput *result = new CAudioEffecterOutput(pFilter);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CAudioEffecterOutput::Construct()
{
    return ME_OK;
}

CAudioEffecterOutput::~CAudioEffecterOutput()
{
    AMLOG_DESTRUCTOR("~CAudioEffecterOutput.\n");
}


