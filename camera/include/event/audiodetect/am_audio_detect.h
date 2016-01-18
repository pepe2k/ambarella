/*
 * audio_detect.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 19/12/2013 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, oi transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AM_AUDIO_DETECT_H__
#define __AM_AUDIO_DETECT_H__

#include <pthread.h>
#include <pulse/pulseaudio.h>

#ifndef AUDIO_DETECT_TYPE_NUM
#define AUDIO_DETECT_TYPE_NUM (2)
#endif

#ifndef AUDIO_DETECT_RINGBUFFER_LEN
#define AUDIO_DETECT_RINGBUFFER_LEN (64)
#endif

/*
 * This enumeration type is used to indicate the
 * type of message sent by audio detect module.
 * Then corresponding action will be triggered.
 */
typedef enum {
   AM_AUDIO_DETECT_ALARM = 0x01,
   AM_AUDIO_DETECT_ANALY = 0x02,
} AM_AUDIO_DETECT_TYPE;

class  CMutex;
class  AmAudioDetect;
class  AmAudioAlert;
class  AmAudioAnalysis;
struct AudioDetectRingBuffer;

struct PaData {
   AmAudioDetect *adev;
   void        *data;
   PaData(AmAudioDetect *dev, void *userdata) :
      adev(dev),
      data(userdata){}
   PaData() :
      adev(NULL),
      data(NULL){}
};

typedef struct audio_detect_msg_s {
   uint64_t msg_pts;
   uint32_t msg_type;
   uint32_t seq_num;
} audio_detect_msg_t;

typedef int (*audio_detect_callback)(audio_detect_msg_t *msg);

/*
 * Begin to declear C interfaces used for dlopen
 * within event framework
 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Start audio detect module. Initializing operations
 * are embedded in this API.
 *
 * @return: 0 on success
 *         -1 on error
 */
int module_start ();

/*
 * Stop audio detect module.
 *
 * @return: 0 on success
 *         -1 on error
 */
int module_stop ();

/*
 * Destroy audio detect module.
 *
 * @ return: 0 on success
 *          -1 on error
 */
int module_destroy ();

/*
 * Get certain configuration of audio detect module.
 *
 * @param config: store return value
 *
 * @return: 0 on success
 *         -1 on error
 */
int module_get_config (MODULE_CONFIG *config);

/*
 * Set certain configuration of audio detect module
 *
 * This API is a little different. User may call it
 * before running or in the time of running and some
 * properties can be changed dynamically and some
 * are not supported.
 *
 * @param config: configuration needs to be set
 *
 * @return: 0 on success
 *         -1 on error
 *          1 on not supporting dynamically
 */
int module_set_config (MODULE_CONFIG *config);

/*
 * Fetch identifier of audio detect module.
 *
 * @return: an enumeration value
 */
MODULE_ID module_get_id(void);

#ifdef __cplusplus
}
#endif

class AmAudioDetect {
public:
   static AmAudioDetect *GetInstance ();
   int AudioDetectInit ();
   void AudioDetectFini ();
   int AudioDetectStart ();
   void AudioDetectStop ();
   void SetAudioSampleRate (uint32_t sampleRate);
   void SetAudioChannel (uint32_t channel);
   void SetAudioChunkBytes (uint32_t chunkTypes);
   int  GetAudioDetectProperty (MODULE_CONFIG *config);
   virtual ~AmAudioDetect ();

public:
   void PaState (pa_context *context, void *data);
   void PaServerInfo (pa_context *context,
                     const pa_server_info *info,
                     void *data);
   void PaRead (pa_stream *stream, size_t bytes, void *data);
   void PaOverflow (pa_stream *stream, void *data);

private:
   AmAudioDetect ();
   int Construct ();
   uint64_t GetCurrentPTS ();
   uint32_t GetLcm (uint32_t a, uint32_t b);
   uint32_t GetAvailDataSize ();
   int AudioAlertInit ();

private:
   static void StaticPaState (pa_context *context, void *data);
   static void StaticPaServerInfo (pa_context *context,
                                   const pa_server_info *info,
                                   void  *data);
   static void StaticPaRead (pa_stream *stream, size_t bytes, void *data);
   static void StaticPaOverflow (pa_stream *stream, void *data);
   static void *AudioEventHandler (void *arg);
   static void CleanUp (void *arg);

public:
   pthread_mutex_t        mRingBufferMutex;
   pthread_cond_t         mRingBufferCond;
   AudioDetectRingBuffer *mpRingBuffer;
   AmAudioAlert          *mpAudioAlert;
   AmAudioAnalysis       *mpAudioAnalysis;
   audio_detect_callback  mAudioDetectCallback[AUDIO_DETECT_TYPE_NUM];
   bool                   mbEnableAlertDetect;
   bool                   mbEnableAnalysisDetect;
   bool                   mbAudioDetectRun;

private:
   pa_threaded_mainloop  *mpPaThreadedMainloop;
   pa_mainloop_api       *mpPaMainloopApi;
   pa_context            *mpPaContext;
   pa_stream             *mpPaStreamRecord;
   PaData                *mpReadData;
   PaData                *mpOverflow;
   uint8_t               *mpAudioBuffer;
   uint8_t               *mpAudioRdPtr;
   uint8_t               *mpAudioWtPtr;
   char                  *mpDefSrcName;
   CMutex                *mpMutex;
   pa_sample_spec         mPaSampleSpec;
   pa_channel_map         mPaChannelMap;
   pa_context_state       mPaContextState;
   uint32_t               mAudioSampleRate;
   uint32_t               mAudioChannel;
   uint32_t               mAudioCodecType;
   uint32_t               mAudioChunkBytes;
   uint32_t               mAudioBufSize;
   uint32_t               mFrameBytes;
   uint64_t               mLastPts;
   uint64_t               mFragmentPts;
   int                    mHwTimerFd;
   bool                   mbCtxConnected;
   pthread_t              mHandlerThreadId;

private:
   static AmAudioDetect  *mpAudioDetectInstance;
};

#endif
