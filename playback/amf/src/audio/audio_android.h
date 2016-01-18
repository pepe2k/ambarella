
#ifndef __AUDIO_ANDROID_H__
#define __AUDIO_ANDROID_H__

#include <media/MediaPlayerInterface.h>
#include <media/AudioRecord.h>
#include <media/mediarecorder.h>
using namespace android;

class CAudioAndroid: public CObject, public IAudioHAL
{
    typedef CObject inherited;
public:
    static IAudioHAL* Create(IEngine* pEngine, AM_INT isSink, AM_INT sourcetype = AUDIO_SOURCE_DEFAULT);

private:
    CAudioAndroid(IEngine* pEngine, AM_INT isSink, AM_INT sourcetype = AUDIO_SOURCE_DEFAULT):
        mpEngine(pEngine),
        mpAudioSink(NULL),
        mpAudioSource(NULL),
        mSamplerate(0),
        mChannels(2),
        mSampleFormat(AudioSystem::PCM_16_BIT),
        mIsSink(isSink),
        mToTSample(0)
    {
        mAudioSourceType = sourcetype;
    }
    AM_ERR Construct();
    virtual ~CAudioAndroid();

public:
        // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete();

    // IAudioHAL
    virtual AM_ERR OpenAudioHAL(AudioParam* pParam, AM_UINT *pLatency, AM_UINT *pAudiosize);
    virtual AM_ERR CloseAudioHAL();
    virtual AM_ERR Render(AM_U8 *pData, AM_UINT dataSize, AM_UINT *pNumFrames);
    virtual AM_ERR ReadStream(AM_U8 *pData, AM_UINT dataSize, AM_UINT *pNumFrames, AM_U64 *pTimeStamp);
    virtual AM_ERR Start() {return ME_OK;} // not implemented yet
    virtual AM_ERR Pause();
    virtual AM_ERR Resume();
    virtual AM_ERR Stop();
    virtual AM_ERR Flush();
    virtual AM_BOOL NeedAVSync();

    virtual AM_UINT GetChunkSize();
    virtual AM_UINT GetBitsPerFrame();

private:
    IEngine* mpEngine;
    MediaPlayerInterface::AudioSink* mpAudioSink;
    AudioRecord* mpAudioSource;

    AM_UINT mSamplerate;
    AM_UINT mChannels;
    AudioSystem::audio_format mSampleFormat;

    AM_INT mIsSink;
    AM_INT mAudioSourceType;
    AM_U64 mToTSample;
};

#endif

