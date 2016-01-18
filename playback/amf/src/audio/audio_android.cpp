#include "am_new.h"
#include "am_types.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_util.h"
#include "pbif.h"

#include "audio_if.h"
#include "audio_android.h"

#define DAudioMaxBufferSize 2048

IAudioHAL* CAudioAndroid::Create(IEngine* pEngine, AM_INT isSink, AM_INT sourcetype)
{
    CAudioAndroid* result = new CAudioAndroid(pEngine, isSink, sourcetype);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CAudioAndroid::Construct()
{

    DSetModuleLogConfig(LogModuleAudioOutputAndroid);
    if (mIsSink) {
        mpAudioSink = (MediaPlayerInterface::AudioSink*)mpEngine->QueryEngineInterface(IID_IAudioSink);
        if(mpAudioSink == NULL)
            return ME_IO_ERROR;
    } else {

    }
    return ME_OK;
}

CAudioAndroid::~CAudioAndroid()
{
    if (!mIsSink) {
        AM_ASSERT(!mpAudioSink);
        if (mpAudioSource) {
            AMLOG_WARN("before delete mpAudioSource 1.\n");
            delete mpAudioSource;
            AMLOG_WARN("after delete mpAudioSource 1.\n");
            mpAudioSource = NULL;
        }
    }
}

void CAudioAndroid::Delete()
{
    if (!mIsSink) {
        AM_ASSERT(!mpAudioSink);
        if (mpAudioSource) {
            AMLOG_WARN("before delete mpAudioSource 2.\n");
            delete mpAudioSource;
            AMLOG_WARN("after delete mpAudioSource 2.\n");
            mpAudioSource = NULL;
        }
    }
    inherited::Delete();
}

void* CAudioAndroid::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IAudioHAL){
        return (IAudioHAL*)this;
    }else{
        return inherited::GetInterface(refiid);
    }
}

AM_ERR CAudioAndroid::OpenAudioHAL(AudioParam* pParam, AM_UINT *pLatency, AM_UINT *pAudiosize)
{
    AM_UINT flags;
    status_t res;
    AM_ASSERT(pParam);

    mSamplerate = pParam->sampleRate;
    mChannels = pParam->numChannels;

    switch (pParam->sampleFormat) {
    case IAudioHAL::FORMAT_S16_LE:
        mSampleFormat = AudioSystem::PCM_16_BIT;
        break;
    case IAudioHAL::FORMAT_U8_LE:
        mSampleFormat = AudioSystem::PCM_8_BIT;
        break;
    default:
        AMLOG_INFO("CAudioAndroid: AudioSampleFormat %d is wrong\n", pParam->sampleFormat);
        return ME_ERROR;
    }

    if (mIsSink) {
        if (mpAudioSink->open(mSamplerate, mChannels, mSampleFormat, 2, NULL, NULL) != NO_ERROR) {
            AMLOG_ERROR("CAudioAndroid: mpAudioSink open failed, Samplerate[%d], Channels[%d],SampleFormat[%d]\n",
                mSamplerate, mChannels, (AM_INT)mSampleFormat);
            return ME_ERROR;
        }
        else
            AMLOG_INFO("CAudioAndroid: mpAudioSink open GOOD, Samplerate[%d], Channels[%d],SampleFormat[%d]\n",
                mSamplerate, mChannels, (AM_INT)mSampleFormat);

        *pAudiosize = mpAudioSink->bufferSize();
        //audio latency
        *pLatency = mpAudioSink->latency();
        *pLatency *= 90;//hard  code here, to do
        AMLOG_INFO("CAudioAndroid latency = %u, audiosize=%u\n", *pLatency, *pAudiosize);
        mpAudioSink->start();
    } else {
        AM_WARNING("before new AudioRecord: mAudioSourceType %d, mSamplerate %d, mChannels %d.\n", mAudioSourceType, mSamplerate, mChannels);
        flags = AudioRecord::RECORD_AGC_ENABLE | AudioRecord::RECORD_NS_ENABLE | AudioRecord::RECORD_IIR_ENABLE;

        //mAudioSourceType = AUDIO_SOURCE_MIC;
        mpAudioSource = new AudioRecord(mAudioSourceType, mSamplerate,
            AudioSystem::PCM_16_BIT,
            (mChannels > 1) ? AudioSystem::CHANNEL_IN_STEREO : AudioSystem::CHANNEL_IN_MONO,
            0 /*4 * DAudioMaxBufferSize /sizeof(AM_U16)*/, flags); //android will query buffer size in alsa, and fill proper value, hard code buffer size will cause some problem

        if (!mpAudioSource) {
            AM_ERROR("new AudioRecord fail.\n");
            return ME_ERROR;
        }
        res = mpAudioSource->initCheck();
        if (res == NO_ERROR) {
            res = mpAudioSource->start();
            if (res != NO_ERROR) {
                AM_ERROR("mpAudioSource->start() fail ret %d.\n", res);
                delete mpAudioSource;
                mpAudioSource = NULL;
                return ME_ERROR;
            }
        } else {
            AM_ERROR("mpAudioSource->initCheck() fail ret %d.\n", res);
            delete mpAudioSource;
            mpAudioSource = NULL;
            return ME_ERROR;
        }
    }

    //mToTSample = 0;
    return ME_OK;
}

AM_ERR CAudioAndroid::CloseAudioHAL()
{
    if (mIsSink) {

    } else {
        AM_ASSERT(mpAudioSource);
        if (mpAudioSource) {
            delete mpAudioSource;
            mpAudioSource = NULL;
        }
    }
    return ME_OK;
}

AM_ERR CAudioAndroid::ReadStream(AM_U8 *pData, AM_UINT dataSize, AM_UINT *pNumFrames, AM_U64 *pTimeStamp)
{
    //tmp modify for return 0
    AM_UINT read_bytes;
    ssize_t ret;
    AM_ASSERT(!mIsSink);
    AM_ASSERT(mpAudioSource);
    if (!mpAudioSource) {
        AM_ERROR("NULL pointer in CAudioAndroid::ReadStream.\n");
        *pNumFrames = 0;
        return ME_ERROR;
    }

    ret = mpAudioSource->read((void*)pData, dataSize);
    read_bytes = dataSize;
    *pNumFrames = read_bytes/mChannels/2; //hard code here: sizeof(s16)=2
    AMLOG_DEBUG("CAudioAndroid::ReadStream datasize %d, ret %d, *pNumFrames %d.\n", dataSize, (AM_UINT)ret, *pNumFrames);
    //pts to with 90khz unit
    *pTimeStamp = (mToTSample*IParameters::TimeUnitDen_90khz)/mSamplerate;
    mToTSample += *pNumFrames;
    return ME_OK;
}

AM_BOOL CAudioAndroid::NeedAVSync()
{
    return AM_TRUE;
}

AM_ERR CAudioAndroid::Pause()
{
    if (mIsSink) {
        AM_ASSERT(mpAudioSink);
        mpAudioSink->pause();
    } else {
        AM_ASSERT(mpAudioSource);
    }
    return ME_OK;
}

AM_ERR CAudioAndroid::Resume()
{
    if (mIsSink) {
        AM_ASSERT(mpAudioSink);
        mpAudioSink->start();
    } else {
        AM_ASSERT(mpAudioSource);
    }
    return ME_OK;
}

AM_ERR CAudioAndroid::Stop()
{
    if (mIsSink) {
        AM_ASSERT(mpAudioSink);
        mpAudioSink->flush();
        mpAudioSink->stop();
        mpAudioSink->close();
    } else {
        AM_ASSERT(mpAudioSource);
        if(mpAudioSource){
            mpAudioSource->stop();
        }
    }
    return ME_OK;
}

AM_ERR CAudioAndroid::Flush()
{
    if (mIsSink) {
        AM_ASSERT(mpAudioSink);
        mpAudioSink->flush();
    } else {
        AM_ASSERT(mpAudioSource);
    }
    return ME_OK;
}

AM_UINT CAudioAndroid::GetChunkSize()
{
    return 0;
}

AM_UINT CAudioAndroid::GetBitsPerFrame()
{
    if (mSampleFormat == AudioSystem::PCM_16_BIT) {
        return (sizeof(AM_S16)*8*mChannels);
    } else if (mSampleFormat == AudioSystem::PCM_8_BIT) {
        return (sizeof(AM_U8)*8*mChannels);
    } else {
        AM_ERROR("Must not comes here, BAD mSampleFormat %lld.\n", mSampleFormat);
        //must not comes here
        mSampleFormat = AudioSystem::PCM_16_BIT;
        return (sizeof(AM_S16)*8*mChannels);
    }
}

AM_ERR CAudioAndroid::Render(AM_U8 *pData, AM_UINT dataSize, AM_UINT *pNumFrames)
{
    AM_UINT numFrames;

    AM_ASSERT(!mpAudioSource);
    AM_ASSERT(mIsSink);
    AM_ASSERT(mpAudioSink);

    ssize_t length = mpAudioSink->write(pData, dataSize);
    if (length != ((ssize_t)dataSize)){
        numFrames = dataSize / mChannels;
        if(mSampleFormat == AudioSystem::PCM_16_BIT) {
            numFrames /= 2;
        }
        *pNumFrames = numFrames;
        return ME_ERROR;
    }

    numFrames = dataSize / mChannels;

    if(mSampleFormat == AudioSystem::PCM_16_BIT) {
            numFrames /= 2;
    }

    *pNumFrames = numFrames;
    return ME_OK;
}

