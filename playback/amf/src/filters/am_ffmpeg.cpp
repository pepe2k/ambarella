#include "am_ffmpeg.h"

//global mutex for ffmpeg init
#include "pthread.h"
extern "C"{
    #include "libavformat/avformat.h"
    #include "libavcodec/avcodec.h"
};

static pthread_mutex_t s_ff_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_ff_lock_mutex = PTHREAD_MUTEX_INITIALIZER;

extern "C" {
    static int ff_lockmgr(void **mutex, enum AVLockOp op)
    {
        pthread_mutex_t** pmutex = (pthread_mutex_t**) mutex;
        switch (op) {
        case AV_LOCK_CREATE:
            *pmutex = &s_ff_lock_mutex;
            break;
        case AV_LOCK_OBTAIN:
            pthread_mutex_lock(*pmutex);
            break;
        case AV_LOCK_RELEASE:
            pthread_mutex_unlock(*pmutex);
            break;
        case AV_LOCK_DESTROY:
            break;
        }
        return 0;
    }
};

void mw_ffmpeg_init()
{
    pthread_mutex_lock(&s_ff_mutex);
    {
         static int s_initialized = 0;
         if(!s_initialized){
             av_lockmgr_register(ff_lockmgr);
             avcodec_init();
             av_register_all();
             s_initialized = 1;
         }
    }
    pthread_mutex_unlock(&s_ff_mutex);
}

