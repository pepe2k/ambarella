/**
 * ain_mic.cpp
 *
 * History:
 *    2008/3/12 - [Cao Rongrong] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <byteswap.h>

#include "am_types.h"
#include "am_new.h"

#include "audio.h"

//-----------------------------------------------------------------------
//
// CamAudio
//
//-----------------------------------------------------------------------

CamAudio::~CamAudio()
{
    if(_fd > 0){
        close(_fd);
        _fd = -1;
    }
    PcmDeinit();
}

AM_ERR CamAudio::PcmInit(snd_pcm_stream_t stream)
{
    AM_INT err;

    mStream = stream;
    err = snd_output_stdio_attach(&_log, stderr, 0);
    AM_ASSERT(err >= 0);

    err = snd_pcm_open(&mpHandle, "plughw:0,0", mStream, /*SND_PCM_NONBLOCK*//*SND_PCM_ASYNC*/0);//SND_PCM_NONBLOCK
    if (err < 0) {
        AM_ERROR("Capture audio open error: %s\n",snd_strerror(err));
    	return ME_ERROR;
    }

    AM_PRINTF("Open %s Audio Device Successful\n",
            mStream == SND_PCM_STREAM_PLAYBACK ? "Playback":"Capture");

    //mChunkSize = 1024;

    return ME_OK;
}

AM_ERR CamAudio::PcmDeinit()
{
    if(_log != NULL){
    snd_output_close(_log);
    _log = NULL;
    }
    if (mpHandle != NULL){
        snd_pcm_close(mpHandle);
        mpHandle = NULL;
    }
}

AM_ERR CamAudio::SetParams(snd_pcm_format_t pcmFormat, AM_UINT sampleRate, AM_UINT numOfChannels)
{
    AM_INT err;

    snd_pcm_hw_params_t *params;
    snd_pcm_sw_params_t *swparams;
    snd_pcm_uframes_t buffer_size;
    AM_UINT start_threshold, stop_threshold;

    AM_UINT period_time = 0;
    AM_UINT buffer_time = 0;

    mPcmFormat = pcmFormat;
    mSampleRate = sampleRate;
    mNumOfChannels = numOfChannels;

    snd_pcm_hw_params_alloca(&params);
    snd_pcm_sw_params_alloca(&swparams);


    err = snd_pcm_hw_params_any(mpHandle, params);
    if (err < 0) {
    	AM_ERROR("Broken configuration for this PCM: no configurations available\n");
    	return ME_ERROR;
    }

    err = snd_pcm_hw_params_set_access(mpHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
    	AM_ERROR("Access type not available\n");
    	return ME_ERROR;
    }

    err = snd_pcm_hw_params_set_format(mpHandle, params, mPcmFormat);
    if (err < 0) {
    	AM_ERROR("Sample format non available\n");
    	return ME_ERROR;
    }

    err = snd_pcm_hw_params_set_channels(mpHandle, params, mNumOfChannels);
    if (err < 0) {
    	AM_ERROR("Channels count non available\n");
    	return ME_ERROR;
    }

    err = snd_pcm_hw_params_set_rate_near(mpHandle, params, &mSampleRate, 0);
    AM_ASSERT(err >= 0);

    err = snd_pcm_hw_params_get_buffer_time_max(params, &buffer_time, 0);
    AM_ASSERT(err >= 0);
    if (buffer_time > 500000)
        buffer_time = 500000;

    period_time = buffer_time / 4;

    // set period size to 1024 frames, to meet aac encoder requirement
    mChunkSize = 1024;
    err = snd_pcm_hw_params_set_period_size(mpHandle, params, mChunkSize, 0);
    AM_ASSERT(err >= 0);
//    err = snd_pcm_hw_params_set_period_time_near(mpHandle, params, &period_time, 0);
//    AM_ASSERT(err >= 0);
    
    err = snd_pcm_hw_params_set_buffer_time_near(mpHandle, params, &buffer_time, 0);
    AM_ASSERT(err >= 0);

    //snd_pcm_wait(mpHandle, 1000);
    err = snd_pcm_hw_params(mpHandle, params);
    if (err < 0) {
    	AM_ERROR("Unable to install hw params:\n");
    	snd_pcm_hw_params_dump(params, _log);
        return ME_ERROR;
    }

    snd_pcm_hw_params_get_period_size(params, &mChunkSize, 0);
    snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
    if (mChunkSize == buffer_size) {
    	AM_ERROR("Can't use period equal to buffer size (%d == %d)\n",
    	      (AM_INT)mChunkSize, (AM_INT)buffer_size);
    	return ME_ERROR;
    }

    snd_pcm_sw_params_current(mpHandle, swparams);

    err = snd_pcm_sw_params_set_avail_min(mpHandle, swparams, mChunkSize);

    /* round up to closest transfer boundary */

    if(mStream == SND_PCM_STREAM_PLAYBACK)
        start_threshold = (buffer_size / mChunkSize) * mChunkSize;
    else
        start_threshold = 1;

    err = snd_pcm_sw_params_set_start_threshold(mpHandle, swparams, start_threshold);
    AM_ASSERT(err >= 0);

    stop_threshold = buffer_size;

    err = snd_pcm_sw_params_set_stop_threshold(mpHandle, swparams, stop_threshold);
    AM_ASSERT(err >= 0);

    if (snd_pcm_sw_params(mpHandle, swparams) < 0) {
    	AM_ERROR("unable to install sw params:\n");
    	snd_pcm_sw_params_dump(swparams, _log);
    	return ME_ERROR;
    }

    if (VERBOSE)
    	snd_pcm_dump(mpHandle, _log);

    mBitsPerSample = snd_pcm_format_physical_width(mPcmFormat);
    mBitsPerFrame = mBitsPerSample * mNumOfChannels;
    mChunkBytes = mChunkSize * mBitsPerFrame / 8;

    AM_PRINTF("chunk_size = %d, chunk_bytes = %d, buffer_size = %d\n", (AM_INT)mChunkSize, (AM_INT)mChunkBytes, buffer_size);

    return ME_OK;
}

AM_UINT CamAudio::PcmRead(AM_U8*data, AM_INT timeout)
{
    AM_ERR err;
    AM_INT r;
    AM_U32 result = 0;
    AM_U32 count = mChunkSize;

    while (count > 0) {
    	r = snd_pcm_readi(mpHandle, data, count);
        if (r == -EAGAIN || (r >= 0 && (AM_UINT)r < count)) {
        	snd_pcm_wait(mpHandle, timeout);
        } else if (r == -EPIPE) {
            err = Xrun(SND_PCM_STREAM_CAPTURE);
            if(err != ME_OK)
                return -1;
        } else if (r == -ESTRPIPE) {
            err = Suspend();
        	if(err != ME_OK)
        	    return -1;
        } else if (r < 0) {
        	AM_ERROR("Read error: %s(%d)\n", snd_strerror(r), r);
        	return -1;
        }
        if (r > 0) {
        	result += r;
        	count -= r;
        	data += r * mBitsPerFrame / 8;      // 4 bytes per frame
        }
    }

    return result;
}


// I/O error handler /
AM_ERR CamAudio::Xrun(snd_pcm_stream_t stream)
{
    snd_pcm_status_t *status;
    int res;

    snd_pcm_status_alloca(&status);
    if ((res = snd_pcm_status(mpHandle, status))<0) {
        AM_ERROR("status error: %s\n", snd_strerror(res));
        return ME_ERROR;
    }

    if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
        AM_PRINTF("%s!!!\n",
        stream == SND_PCM_STREAM_PLAYBACK ? "Playback underrun":"Capture overrun");

        if (VERBOSE) {
            AM_PRINTF("Status:\n");
            snd_pcm_status_dump(status, _log);
        }
        if ((res = snd_pcm_prepare(mpHandle))<0) {
            AM_ERROR("xrun: prepare error: %s", snd_strerror(res));
            return ME_ERROR;
        }
        return ME_OK;		// ok, data should be accepted again 
    }

    if (snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING) {
        if (VERBOSE) {
            AM_PRINTF("Status(DRAINING):\n");
            snd_pcm_status_dump(status, _log);
        }
            AM_PRINTF("capture stream format change? attempting recover...\n");
            if ((res = snd_pcm_prepare(mpHandle))<0) {
            AM_ERROR("xrun(DRAINING): prepare error: %s\n", snd_strerror(res));
            return ME_ERROR;
        }
        return ME_OK;
    }
    if (VERBOSE) {
        AM_PRINTF("Status(R/W):\n");
        snd_pcm_status_dump(status, _log);
    }
    AM_ERROR("read/write error, state = %s\n", snd_pcm_state_name(snd_pcm_status_get_state(status)));
    return ME_ERROR;
}



AM_ERR CamAudio::Suspend(void)
{
    int res;

    AM_PRINTF("Suspended. Trying resume. ");
    while ((res = snd_pcm_resume(mpHandle)) == -EAGAIN)
    ::sleep(1);	/* wait until suspend flag is released */
    if (res < 0) {
        AM_PRINTF("Failed. Restarting stream. ");
        if ((res = snd_pcm_prepare(mpHandle)) < 0) {
            AM_ERROR("suspend: prepare error: %s", snd_strerror(res));
            return ME_ERROR;
        }
    }
    AM_PRINTF("Suspend Done.\n");
    return ME_OK;
}


AM_ERR CamAudio::ShowHeader(snd_pcm_stream_t stream)
{
    AM_PRINTF("Audio: %s, Format: %s, Rate: %d Hz, Channel: %s\n",
        stream == SND_PCM_STREAM_PLAYBACK ? "Playback":"Capture",
        snd_pcm_format_name(mPcmFormat), mSampleRate,
        mNumOfChannels == 1 ? "Mono":"Stereo");

    return ME_OK;
}

AM_ERR CamAudio::Wave_Header(AM_U32 cnt)
{
    WaveHeader h;
    WaveFmtBody f;
    WaveChunkHeader cf, cd;
    AM_INT bits;
    AM_UINT tmp;
    AM_U16 tmp2;

    /* WAVE cannot handle greater than 32bit (signed?) int */
    if (cnt == (size_t)-2)
    	cnt = 0x7fffff00;


	bits = 8;
	switch ((unsigned long) mPcmFormat) {
	case SND_PCM_FORMAT_U8:
		bits = 8;
		break;
	case SND_PCM_FORMAT_S16_LE:
		bits = 16;
		break;
	case SND_PCM_FORMAT_S32_LE:
		bits = 32;
		break;
	case SND_PCM_FORMAT_S24_LE:
	case SND_PCM_FORMAT_S24_3LE:
		bits = 24;
		break;
	default:
		AM_ERROR("Wave doesn't support %s format...\n", snd_pcm_format_name(mPcmFormat));
		return ME_ERROR;
	}

    h.magic = WAV_RIFF;
    tmp = cnt + sizeof(WaveHeader) + sizeof(WaveChunkHeader) + sizeof(WaveFmtBody) + sizeof(WaveChunkHeader) - 8;
    h.length = LE_INT(tmp);
    h.type = WAV_WAVE;

    cf.type = WAV_FMT;
    cf.length = LE_INT(16);

    f.format = LE_SHORT(WAV_PCM_CODE);
    f.modus = LE_SHORT(mNumOfChannels);
    f.sample_fq = LE_INT(mSampleRate);

    tmp2 = mNumOfChannels * snd_pcm_format_physical_width(mPcmFormat) / 8;
    f.byte_p_spl = LE_SHORT(tmp2);
    tmp = (u_int) tmp2 * mSampleRate;

    f.byte_p_sec = LE_INT(tmp);
    f.bit_p_spl = LE_SHORT(bits);

    cd.type = WAV_DATA;
    cd.length = LE_INT(cnt);

    if (::write(_fd, &h, sizeof(WaveHeader)) != sizeof(WaveHeader) ||
        ::write(_fd, &cf, sizeof(WaveChunkHeader)) != sizeof(WaveChunkHeader) ||
        ::write(_fd, &f, sizeof(WaveFmtBody)) != sizeof(WaveFmtBody) ||
        ::write(_fd, &cd, sizeof(WaveChunkHeader)) != sizeof(WaveChunkHeader)) {
    	AM_ERROR("write error\n");
    	return ME_IO_ERROR;
    }

    return ME_OK;
}

AM_ERR CamAudio::Wave_End(void)
{				/* only close output */
    WaveChunkHeader cd;
    AM_S64 length_seek;
    unsigned long long filelen;
    u_int rifflen;

    length_seek = sizeof(WaveHeader) + sizeof(WaveChunkHeader) + sizeof(WaveFmtBody);
    cd.type = WAV_DATA;
    cd.length = _fdcount > 0x7fffffff ? LE_INT(0x7fffffff) : LE_INT(_fdcount);
    filelen = _fdcount + 2*sizeof(WaveChunkHeader) + sizeof(WaveFmtBody) + 4;
    rifflen = filelen > 0x7fffffff ? LE_INT(0x7fffffff) : LE_INT(filelen);
    if (lseek(_fd, 4, SEEK_SET) == 4)
    	write(_fd, &rifflen, 4);
    if (lseek(_fd, length_seek, SEEK_SET) == length_seek)
    	write(_fd, &cd, sizeof(WaveChunkHeader));

    close(_fd);
    
    return ME_OK;
}

AM_ERR CamAudio::Au_Header(AM_U32 cnt)
{
    AuHeader ah;

    ah.magic = AU_MAGIC;
    ah.hdr_size = BE_INT(24);
    ah.data_size = BE_INT(cnt);
    switch ((unsigned long) mPcmFormat) {
    case SND_PCM_FORMAT_MU_LAW:
    	ah.encoding = BE_INT(AU_FMT_ULAW);
    	break;
    case SND_PCM_FORMAT_U8:
    	ah.encoding = BE_INT(AU_FMT_LIN8);
    	break;
    case SND_PCM_FORMAT_S16_BE:
    	ah.encoding = BE_INT(AU_FMT_LIN16);
    	break;
    default:
    	AM_ERROR("Sparc Audio doesn't support %s format...", snd_pcm_format_name(mPcmFormat));
    	return ME_ERROR;
    }
    ah.sample_rate = BE_INT(mSampleRate);
    ah.channels = BE_INT(mNumOfChannels);
    if (::write(_fd, &ah, sizeof(AuHeader)) != sizeof(AuHeader)) {
    	AM_ERROR("write error");
    	return ME_IO_ERROR;
    }

    return ME_OK;
}

AM_ERR CamAudio::Au_End(void)
{
    AuHeader ah;
    AM_S64 length_seek;

    length_seek = (char *)&ah.data_size - (char *)&ah;
    ah.data_size = _fdcount > 0xffffffff ? 0xffffffff : BE_INT(_fdcount);
    if (::lseek(_fd, length_seek, SEEK_SET) == length_seek)
    	::write(_fd, &ah.data_size, sizeof(ah.data_size));

    close(_fd);

    return ME_OK;
}


