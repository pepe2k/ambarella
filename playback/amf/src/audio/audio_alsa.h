#ifndef __AUDIO_ALSA_H__
#define __AUDIO_ALSA_H__

#define VERBOSE 	0

class CAudioALSA: public IAudioHAL
{
public:
    static IAudioHAL* Create();

private:
    CAudioALSA():
        mpAudioHandle(NULL),
        mOpened(false),
        mStream(SND_PCM_STREAM_PLAYBACK),
        mPcmFormat(SND_PCM_FORMAT_S16_LE),
        mSampleRate(48000),
        mNumOfChannels(2),
        mChunkSize(1024),
        mChunkBytes(128000),
        mpLog(NULL),
        mbNoWait(true),
        mpPreLoadData(NULL),
        mPreLoadedSize(0),
        mToTSample(0)
    { }
    AM_ERR Construct();
    virtual ~CAudioALSA();

public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete();

    // IAudioHAL
    virtual AM_ERR OpenAudioHAL(AudioParam* pParam, AM_UINT *pLatency, AM_UINT *pAudiosize);
    virtual AM_ERR CloseAudioHAL();
    virtual AM_ERR Render(AM_U8 *pData, AM_UINT dataSize, AM_UINT *pNumFrames);
    virtual AM_ERR ReadStream(AM_U8 *pData, AM_UINT dataSize, AM_UINT *pNumFrames, AM_U64 *pTimeStamp);
    virtual AM_ERR Start();
    virtual AM_ERR Pause();
    virtual AM_ERR Resume();
    virtual AM_ERR Stop();
    virtual AM_ERR Flush();
    virtual AM_BOOL NeedAVSync();

    virtual AM_UINT GetChunkSize() {  return (AM_UINT)mChunkSize;  }
    virtual AM_UINT GetBitsPerFrame() {  return mBitsPerFrame;  }

private:
    AM_ERR PcmInit(snd_pcm_stream_t stream);
    AM_ERR PcmDeinit();
    AM_ERR SetParams(snd_pcm_format_t pcmFormat, AM_UINT sampleRate, AM_UINT numOfChannels);
    AM_ERR Xrun(snd_pcm_stream_t stream);
    AM_ERR Suspend(void);
    AM_INT PcmWrite(AM_U8 *pData, AM_UINT rcount);
    AM_INT PcmRead(AM_U8 *pData, AM_UINT count);
    void gettimestamp(snd_pcm_t *handle, snd_timestamp_t *timestamp);

private:
    snd_pcm_t* mpAudioHandle;
    bool mOpened;
    snd_pcm_stream_t mStream;
    snd_pcm_format_t mPcmFormat;
    AM_UINT mSampleRate;
    AM_UINT mNumOfChannels;
    snd_pcm_uframes_t mChunkSize;
    AM_UINT  mChunkBytes;
    AM_UINT mBitsPerSample;
    AM_UINT mBitsPerFrame;
    snd_output_t *mpLog;
    bool mbNoWait;
    snd_timestamp_t mStartTimeStamp;
    AM_U8* mpPreLoadData;
    AM_U64 mPreLoadedSize;

private:
    AM_U64 mToTSample;
};

#endif

