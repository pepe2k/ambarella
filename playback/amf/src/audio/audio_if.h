#ifndef __AUDIO_IF_H__
#define __AUDIO_IF_H__

class IAudioHAL : public IInterface
{
public:
    enum STREAM {
        STREAM_PLAYBACK = 0,
        STREAM_CAPTURE,
    };

    enum PCM_FORMAT {
        FORMAT_S16_LE = 0,
        FORMAT_U8_LE,
        FORMAT_S16_BE,//noable
    };

    struct AudioParam
    {
        AM_UINT sampleRate;
        AM_UINT numChannels;
        STREAM stream;
        PCM_FORMAT sampleFormat;
    };

public:
    DECLARE_INTERFACE(IAudioHAL, IID_IAudioHAL);

    virtual AM_ERR OpenAudioHAL(AudioParam* pParam, AM_UINT *pLatency, AM_UINT *pAudiosize) = 0;
    virtual AM_ERR CloseAudioHAL() = 0;
    virtual AM_ERR Render(AM_U8 *pData, AM_UINT dataSize, AM_UINT *pNumFrames) = 0;
    virtual AM_ERR ReadStream(AM_U8 *pData, AM_UINT dataSize, AM_UINT *pNumFrames, AM_U64 *pTimeStamp) = 0;
    virtual AM_ERR Start() = 0;
    virtual AM_ERR Pause() = 0;
    virtual AM_ERR Resume() = 0;
    virtual AM_ERR Stop() = 0;
    virtual AM_ERR Flush() = 0;
    virtual AM_BOOL NeedAVSync() = 0;

    virtual AM_UINT GetChunkSize() = 0;
    virtual AM_UINT GetBitsPerFrame() = 0;
};

IAudioHAL* AM_CreateAudioHAL(IEngine* pEngine, AM_INT isSink, AM_INT audio_source_type);

#endif

