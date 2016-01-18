#include "am_new.h"
#include "am_types.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_util.h"
#include <alsa/asoundlib.h>

#include "audio_if.h"
#include "audio_alsa.h"

#ifndef timersub
#define	timersub(a, b, result) \
do { \
	(result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
	(result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
	if ((result)->tv_usec < 0) { \
		--(result)->tv_sec; \
		(result)->tv_usec += 1000000; \
	} \
} while (0)
#endif

#ifndef timermsub
#define	timermsub(a, b, result) \
do { \
	(result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
	(result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec; \
	if ((result)->tv_nsec < 0) { \
		--(result)->tv_sec; \
		(result)->tv_nsec += 1000000000L; \
	} \
} while (0)
#endif

IAudioHAL* CAudioALSA::Create()
{
    CAudioALSA* result = new CAudioALSA();
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CAudioALSA::Construct()
{
    return ME_OK;
}

CAudioALSA::~CAudioALSA()
{
    PcmDeinit();
    if (mpPreLoadData) {
        free(mpPreLoadData);
        mpPreLoadData = NULL;
        mPreLoadedSize = 0;
    }
}

void CAudioALSA::Delete()
{
    if (mpPreLoadData) {
        free(mpPreLoadData);
        mpPreLoadData = NULL;
        mPreLoadedSize = 0;
    }
    delete this;
}

void *CAudioALSA::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IAudioHAL)
    	return (IAudioHAL*)this;
    return NULL;
}

AM_ERR CAudioALSA::OpenAudioHAL(AudioParam* pParam, AM_UINT *pLatency, AM_UINT *pAudiosize)
{
    snd_pcm_stream_t stream;
    snd_pcm_format_t format;
    AM_ERR err = ME_OK;
    if(mOpened == false) {

        if (pParam->stream == STREAM_PLAYBACK)
            stream = SND_PCM_STREAM_PLAYBACK;
        else
            stream = SND_PCM_STREAM_CAPTURE;

        if (pParam->sampleFormat == FORMAT_S16_LE)
            format = SND_PCM_FORMAT_S16_LE;
        else
            format = SND_PCM_FORMAT_S16_LE;

        if (pParam->sampleFormat == FORMAT_S16_BE)
            format = SND_PCM_FORMAT_S16_BE;

        if(NULL!=pLatency)
        {
            *pLatency = 800*90;//estimate for linux here, to do
        }

        err = PcmInit(stream);
        SetParams(format, pParam->sampleRate, pParam->numChannels);
        mOpened = true;
    }
    return err;
}

AM_ERR CAudioALSA::CloseAudioHAL()
{
    PcmDeinit();
    return ME_OK;
}

AM_BOOL CAudioALSA::NeedAVSync()
{
#if (TARGET_USE_AMBARELLA_I1_DSP||TARGET_USE_AMBARELLA_S2_DSP)
    return AM_TRUE;
#elif TARGET_USE_AMBARELLA_A5S_DSP
    return AM_FALSE;
#endif
}

AM_ERR CAudioALSA::Start()
{
    AM_INT err;
    snd_pcm_status_t *status;

    if(mStream == SND_PCM_STREAM_PLAYBACK ){
        return ME_OK;
    }
    if ((err = snd_pcm_start(mpAudioHandle)) < 0) {
        AM_ERROR("PCM start error: %s\n", snd_strerror(err));
        return ME_ERROR;
    }

    snd_pcm_status_alloca(&status);

    if ((err = snd_pcm_status(mpAudioHandle, status))<0) {
        AM_ERROR("Get PCM status error: %s\n", snd_strerror(err));
        return ME_ERROR;
    }
    snd_pcm_status_get_trigger_tstamp(status, &mStartTimeStamp);

    AM_PRINTF("start time %d:%d\n", (AM_INT)mStartTimeStamp.tv_sec, (AM_INT)mStartTimeStamp.tv_usec);

    return ME_OK;
}

AM_ERR CAudioALSA::Pause()
{
    return ME_OK;
}

AM_ERR CAudioALSA::Resume()
{
    return ME_OK;
}

AM_ERR CAudioALSA::Stop()
{
    return ME_OK;
}

AM_ERR CAudioALSA::Flush()
{
    return ME_OK;
}
//#define _dump_audio_pcm_
//#define _fake_feed_audio_pcm_
AM_ERR CAudioALSA::Render(AM_U8 *pData, AM_UINT dataSize, AM_UINT *pNumFrames)
{
    AM_INT frm_cnt;
#ifdef _dump_audio_pcm_
    static FILE* mpDumpFile=NULL;
    if (!mpDumpFile) {
        mpDumpFile = fopen("/mnt/win7/4kby2k_from_s2/ts/bdpcm.data", "ab");
    }
    if (mpDumpFile) {
        AM_INFO("write audio data, pData=%p, dataSize=%u.\n",pData,dataSize);
        fwrite(pData, 1, (size_t)dataSize, mpDumpFile);
        fclose(mpDumpFile);
        mpDumpFile = NULL;
    } else {
        AM_INFO("open  audio dump file fail.\n");
    }
#endif

#ifdef _fake_feed_audio_pcm_
    static FILE* mpFakeFile=NULL;
    AM_U8 fake_frm[4096];
    if (!mpFakeFile) {
        mpFakeFile = fopen("/mnt/win7/4kby2k_from_s2/ts/bdpcm.data", "rb");
    }
    if (mpFakeFile) {
        AM_INFO("write audio data, pData=%p, dataSize=%u.\n",pData,dataSize);
        fread(fake_frm, 1, 4096, mpFakeFile);
//        fclose(mpFakeFile);
//        mpFakeFile = NULL;
    } else {
        AM_INFO("open  audio fake file fail.\n");
    }
    AM_INFO("PcmWrite count=1024.\n");
    frm_cnt = PcmWrite(fake_frm, 1024);
#else
    if (dataSize >= mChunkBytes) {
        AM_INFO("PcmWrite chunk_size=%u.\n", dataSize * 8 / mBitsPerFrame);
        frm_cnt = PcmWrite(pData, dataSize * 8 / mBitsPerFrame);
        if (frm_cnt <= 0) {
            return ME_ERROR;
        }

        /*
        AM_ASSERT(frm_cnt == mChunkSize);
        if (frm_cnt != mChunkSize) {
            AM_ERROR("frm_cnt %d != mChunkSize %d, dataSize %d.\n", frm_cnt, mChunkSize, dataSize);
        }
        */

        *pNumFrames = frm_cnt;
        return ME_OK;

    } else {
        if (mPreLoadedSize+dataSize<mChunkBytes) {
            memcpy(mpPreLoadData+mPreLoadedSize, pData, dataSize);
            mPreLoadedSize += dataSize;
            AM_INFO("pcm buffering....... mPreLoadedSize=%llu\n", mPreLoadedSize);
            *pNumFrames = 0;
            return ME_OK;
        } else {
            AM_UINT tmp_dataSize=mChunkBytes-mPreLoadedSize;
            AM_ASSERT(tmp_dataSize<=dataSize);
            memcpy(mpPreLoadData+mPreLoadedSize, pData, tmp_dataSize);
            mPreLoadedSize = 0;
            AM_INFO("PcmWrite chunk_size=%lu.\n", mChunkSize);
            frm_cnt = PcmWrite(mpPreLoadData, mChunkSize);
            if (tmp_dataSize < dataSize) {
                memcpy(mpPreLoadData+mPreLoadedSize, pData+tmp_dataSize, dataSize-tmp_dataSize);
                mPreLoadedSize +=  dataSize-tmp_dataSize;
            }
            if (frm_cnt <= 0) {
                return ME_ERROR;
            }
            *pNumFrames = frm_cnt;
            return ME_OK;
        }
    }
#endif

}

AM_ERR CAudioALSA::ReadStream(AM_U8 *pData, AM_UINT dataSize, AM_UINT *pNumFrames, AM_U64 *pTimeStampUs)
{
    AM_INT err;
    snd_pcm_status_t *status;
    AM_INT frm_cnt;
    snd_timestamp_t now, diff, trigger;

    AM_ASSERT(dataSize >= mChunkSize * mBitsPerFrame / 8);      // make sure buffer can hold one chunck of data
    frm_cnt = PcmRead(pData, mChunkSize);

    if (frm_cnt <= 0)
        return ME_ERROR;

    AM_ASSERT(frm_cnt == (AM_INT)mChunkSize);

    *pNumFrames = frm_cnt;

    snd_pcm_status_alloca(&status);
    if ((err = snd_pcm_status(mpAudioHandle, status))<0) {
        AM_ERROR("Get PCM status error: %s\n", snd_strerror(err));
        return ME_ERROR;
    }

    snd_pcm_status_get_tstamp(status, &now);
    snd_pcm_status_get_trigger_tstamp(status, &trigger);

    timersub(&now, &mStartTimeStamp, &diff);
    *pTimeStampUs = diff.tv_sec * 1000000 + diff.tv_usec;

    //pts to with 90khz unit
    *pTimeStampUs = (mToTSample*IParameters::TimeUnitDen_90khz)/mSampleRate;
    mToTSample += frm_cnt;

//    printf("avail %d\n", snd_pcm_status_get_avail(status));
//    printf("PCM status: avail %d, time %d:%d - %d:%d, trigger %d:%d, PTS %lld\n", snd_pcm_status_get_avail(status),
//        now.tv_sec, now.tv_usec, diff.tv_sec, diff.tv_usec, trigger.tv_sec, trigger.tv_usec, *pTimeStampUs);
    return ME_OK;
}

AM_ERR CAudioALSA::PcmInit(snd_pcm_stream_t stream)
{
    AM_INT err;

    mStream = stream;
    err = snd_output_stdio_attach(&mpLog, stderr, 0);
    AM_ASSERT(err >= 0);

    if(mStream == SND_PCM_STREAM_PLAYBACK ){
        //open default to support 1channel for playback.
        err = snd_pcm_open(&mpAudioHandle, "default", mStream, 0);
        if (err < 0) {
            AM_ERROR("Capture audio open error: %s\n", snd_strerror(err));
            return ME_ERROR;
        }
    }else{
        err = snd_pcm_open(&mpAudioHandle, "default", mStream, 0);
        if (err < 0) {
            AM_INFO("snd_pcm_opne by default failed! Use MICALL.\n");
            err = snd_pcm_open(&mpAudioHandle, "MICALL", mStream, 0);
        }
        if (err < 0) {
            AM_ERROR("Capture audio open error: %s\n", snd_strerror(err));
            return ME_ERROR;
        }
    }

    AM_INFO("Open %s Audio Device Successful\n",
            mStream == SND_PCM_STREAM_PLAYBACK ? "Playback":"Capture");

    return ME_OK;
}

AM_ERR CAudioALSA::PcmDeinit()
{
    if(mpLog != NULL){
    snd_output_close(mpLog);
    mpLog = NULL;
    }
    if (mpAudioHandle != NULL){
        snd_pcm_close(mpAudioHandle);
        mpAudioHandle = NULL;
    }

    return ME_OK;
}

AM_ERR CAudioALSA::SetParams(snd_pcm_format_t pcmFormat, AM_UINT sampleRate, AM_UINT numOfChannels)
{
    AM_INT err;

    snd_pcm_hw_params_t *params;
    snd_pcm_sw_params_t *swparams;
    snd_pcm_uframes_t buffer_size;
    AM_UINT start_threshold, stop_threshold;

//    AM_UINT period_time = 0;
    AM_UINT buffer_time = 0;

    mPcmFormat = pcmFormat;
    mSampleRate = sampleRate;
    mNumOfChannels = numOfChannels;

    snd_pcm_hw_params_alloca(&params);
    snd_pcm_sw_params_alloca(&swparams);


    err = snd_pcm_hw_params_any(mpAudioHandle, params);
    if (err < 0) {
        AM_ERROR("Broken configuration for this PCM: no configurations available\n");
        return ME_ERROR;
    }

    err = snd_pcm_hw_params_set_access(mpAudioHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        AM_ERROR("Access type not available\n");
        return ME_ERROR;
    }

    err = snd_pcm_hw_params_set_format(mpAudioHandle, params, mPcmFormat);
    if (err < 0) {
        AM_ERROR("Sample format non available\n");
        return ME_ERROR;
    }

    err = snd_pcm_hw_params_set_channels(mpAudioHandle, params, mNumOfChannels);
    if (err < 0) {
        AM_ERROR("Channels count non available\n");
        return ME_ERROR;
    }

    err = snd_pcm_hw_params_set_rate_near(mpAudioHandle, params, &mSampleRate, 0);
    AM_ASSERT(err >= 0);

    err = snd_pcm_hw_params_get_buffer_time_max(params, &buffer_time, 0);
    AM_ASSERT(err >= 0);
    if (buffer_time > 500000)
        buffer_time = 500000;

//    period_time = buffer_time / 4;

    // set period size to 1024 frames, to meet aac encoder requirement
    mChunkSize = 1024;
    err = snd_pcm_hw_params_set_period_size(mpAudioHandle, params, mChunkSize, 0);
    AM_ASSERT(err >= 0);
//    err = snd_pcm_hw_params_set_period_time_near(mpAudioHandle, params, &period_time, 0);
//    AM_ASSERT(err >= 0);

    err = snd_pcm_hw_params_set_buffer_time_near(mpAudioHandle, params, &buffer_time, 0);
    AM_ASSERT(err >= 0);

    //snd_pcm_wait(mpAudioHandle, 1000);
    err = snd_pcm_hw_params(mpAudioHandle, params);
    if (err < 0) {
        AM_ERROR("Unable to install hw params:\n");
        snd_pcm_hw_params_dump(params, mpLog);
        return ME_ERROR;
    }

    snd_pcm_hw_params_get_period_size(params, &mChunkSize, 0);
    snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
    if (mChunkSize == buffer_size) {
        AM_ERROR("Can't use period equal to buffer size (%d == %d)\n",
            (AM_INT)mChunkSize, (AM_INT)buffer_size);
        return ME_ERROR;
    }

    snd_pcm_sw_params_current(mpAudioHandle, swparams);

    err = snd_pcm_sw_params_set_avail_min(mpAudioHandle, swparams, mChunkSize);

    /* round up to closest transfer boundary */

    if(mStream == SND_PCM_STREAM_PLAYBACK)
        start_threshold = (buffer_size / mChunkSize) * mChunkSize;
    else
        start_threshold = 1;

    err = snd_pcm_sw_params_set_start_threshold(mpAudioHandle, swparams, start_threshold);
    AM_ASSERT(err >= 0);

    stop_threshold = buffer_size;

    err = snd_pcm_sw_params_set_stop_threshold(mpAudioHandle, swparams, stop_threshold);
    AM_ASSERT(err >= 0);

    if (snd_pcm_sw_params(mpAudioHandle, swparams) < 0) {
        AM_ERROR("unable to install sw params:\n");
        snd_pcm_sw_params_dump(swparams, mpLog);
        return ME_ERROR;
    }

    if (VERBOSE)
        snd_pcm_dump(mpAudioHandle, mpLog);

    mBitsPerSample = snd_pcm_format_physical_width(mPcmFormat);
    mBitsPerFrame = mBitsPerSample * mNumOfChannels;
    mChunkBytes = mChunkSize * mBitsPerFrame / 8;
    if (mChunkBytes) {
        mpPreLoadData = (AM_U8*)realloc(mpPreLoadData, mChunkBytes);
        if (!mpPreLoadData) {
            AM_ERROR("realloc buffer for mpPreLoadData failed,  mpPreLoadData=%p\n", mpPreLoadData);
            return ME_ERROR;
        }
        mPreLoadedSize = 0;
    }

    AM_WARNING("chunk_size = %d, chunk_bytes = %d, buffer_size = %d\n", (AM_INT)mChunkSize, (AM_INT)mChunkBytes, (AM_INT)buffer_size);
    AM_INFO("Support Pause? %d\n", snd_pcm_hw_params_can_pause(params));
    return ME_OK;
}


// I/O error handler /
AM_ERR CAudioALSA::Xrun(snd_pcm_stream_t stream)
{
    snd_pcm_status_t *status;
    int err;

    snd_pcm_status_alloca(&status);
    if ((err = snd_pcm_status(mpAudioHandle, status))<0) {
        AM_ERROR("status error: %s\n", snd_strerror(err));
        return ME_ERROR;
    }

    if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
        AM_WARNING("%s!!!\n", stream == SND_PCM_STREAM_PLAYBACK ?
        "Playback underrun":"Capture overrun");

        if ((err = snd_pcm_prepare(mpAudioHandle))<0) {
            AM_ERROR("xrun: prepare error: %s", snd_strerror(err));
            return ME_ERROR;
        }
        return ME_OK;		// ok, data should be accepted again
    }

    if (snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING) {
        AM_INFO("capture stream format change? attempting recover...\n");
        if ((err = snd_pcm_prepare(mpAudioHandle))<0) {
            AM_ERROR("xrun(DRAINING): prepare error: %s\n", snd_strerror(err));
            return ME_ERROR;
        }
        return ME_OK;
    }
    AM_ERROR("read/write error, state = %s\n", snd_pcm_state_name(snd_pcm_status_get_state(status)));
    return ME_ERROR;
}

AM_ERR CAudioALSA::Suspend(void)
{
    int err;

    AM_WARNING("Suspended. Trying resume. ");
    while ((err = snd_pcm_resume(mpAudioHandle)) == -EAGAIN)
    ::sleep(1);	/* wait until suspend flag is released */
    if (err < 0) {
        AM_WARNING("Failed. Restarting stream. ");
        if ((err = snd_pcm_prepare(mpAudioHandle)) < 0) {
            AM_ERROR("suspend: prepare error: %s", snd_strerror(err));
            return ME_ERROR;
        }
    }
    AM_WARNING("Suspend Done.\n");
    return ME_OK;
}

AM_INT CAudioALSA::PcmRead(AM_U8 *pData, AM_UINT rcount)
{
    AM_ERR err;
    AM_INT r;
    AM_U32 result = 0;
    AM_U32 count = rcount;

    if (count != mChunkSize) {
        count = mChunkSize;
    }

    while (count > 0) {
        r = snd_pcm_readi(mpAudioHandle, pData, count);

        if (r == -EAGAIN || (r >= 0 && (AM_UINT)r < count)) {
            if (!mbNoWait)
		snd_pcm_wait(mpAudioHandle, 100);
        } else if (r == -EPIPE) {                   // an overrun occurred
            if ((err = Xrun(SND_PCM_STREAM_CAPTURE)) != ME_OK)
                return -1;
        } else if (r == -ESTRPIPE) {            // a suspend event occurred
            if ((err = Suspend()) != ME_OK)
                return -1;
        } else if (r < 0) {
            if(r == -EIO)
                AM_INFO("-EIO error!\n");
            else if(r == -EINVAL)
                AM_INFO("-EINVAL error!\n");
            else if(r == -EINTR)
                AM_INFO("-EINTR error!\n");
            else
                AM_ERROR("Read error: %s(%d)\n", snd_strerror(r), r);
            return -1;
        }
        if (r > 0) {
            result += r;
            count -= r;
            pData += r * mBitsPerFrame / 8;      // convert frame num to bytes
        }
    }

    return result;
}

AM_INT CAudioALSA::PcmWrite(AM_U8 *pData, AM_UINT count)
{
    AM_ERR err;
    AM_INT r;
    AM_INT result = 0;
    AM_UINT count_bak = count;

    if (count < mChunkSize) {
        snd_pcm_format_set_silence(mPcmFormat, pData + count * mBitsPerFrame/ 8, (mChunkSize - count) * mNumOfChannels);
        count = mChunkSize;
    }
    while (count > 0) {
        r = snd_pcm_writei(mpAudioHandle, pData, count);
        if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
            if (!mbNoWait)
                snd_pcm_wait(mpAudioHandle, 100);
        }
        else if (r == -EPIPE) {                 // an overrun occurred
            if ((err = Xrun(SND_PCM_STREAM_PLAYBACK)) != ME_OK)
                goto __restart_alsa;
	 } else if (r == -ESTRPIPE) {        // a suspend event occurred
	    if ((err = Suspend()) != ME_OK)
                goto __restart_alsa;
	 }else if(r == -EIO || r == -EBADFD){
	     AM_WARNING("PcmWrite -- write error: %s\n", snd_strerror(r));
            goto __restart_alsa;
        } else if (r < 0) {
            AM_WARNING("PcmWrite -- write error: %s\n", snd_strerror(r));
            if(r == -EINVAL)
                AM_WARNING("-EINVAL error!\n");
            else
                AM_WARNING("unknown error!\n");
            return -1;
        }
        if (r > 0) {
            result += r;
            count -= r;
            pData += r * mBitsPerFrame / 8;
        }
        continue;
__restart_alsa:
	 //Somehow the stream is in a bad state. The driver probably has a bug
	 //    and recovery(Xrun() or Suspend()) doesn't seem to handle this.
        AM_WARNING("PcmWrite -- write error: %s, restart \n", snd_strerror(r));
        PcmDeinit();
        PcmInit(mStream);
        SetParams(mPcmFormat,mSampleRate,mNumOfChannels);
        Start();
    }

    if((AM_UINT)result > count_bak){
        result = count_bak;
    }
    return result;
}


