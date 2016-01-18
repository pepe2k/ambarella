
#include "am_new.h"
#include "am_types.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "audio_if.h"
#ifdef PLATFORM_LINUX
#include <alsa/asoundlib.h>
#include "audio_alsa.h"
#endif
#ifdef PLATFORM_ANDROID
#include <media/MediaPlayerInterface.h>
#include "audio_android.h"
#endif

IAudioHAL* AM_CreateAudioHAL(IEngine* pEngine, AM_INT isSink, AM_INT audio_source_type)
{
#ifdef PLATFORM_LINUX
    return (IAudioHAL*)CAudioALSA::Create();
#endif
#ifdef PLATFORM_ANDROID
    return (IAudioHAL*)CAudioAndroid::Create(pEngine, isSink, audio_source_type);
#endif
    return NULL;
}

