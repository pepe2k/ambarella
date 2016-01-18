/*
**
** Copyright 2009, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef ANDROID_HARDWARE_CAMERA_HARDWARE_H
#define ANDROID_HARDWARE_CAMERA_HARDWARE_H
   
#include <camera/CameraHardwareInterface.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <utils/threads.h>
#include <binder/IPCThreadState.h>

namespace android {

class BSBMemoryHeap;

typedef struct {
    unsigned char * pFrame;
    int pic_size;
    unsigned char * pSPS;
    int sps_size;
    unsigned char * pPPS;
    int pps_size;
    int offset;
    unsigned char * pAmbaInfo;
    int ambaInfo_size;
}CSPECIFIC_INFO;


class AmbaMemory : public MemoryBase
{
public:
    AmbaMemory(const sp<IMemoryHeap>& heap, ssize_t offset, size_t size)
        :MemoryBase(heap,offset,size)
    {
    }
    ~AmbaMemory();

public:
    int mIndex;
    int mPTS;
    bool bSpsPps;
    int mStartAddr;
    int mSize;
    CSPECIFIC_INFO mSpecific;
};

#define VIDEO_BUF_COUNT 8

class AMCameraHal : public CameraHardwareInterface
{
public:
    virtual sp<IMemoryHeap> getPreviewHeap() const;
    virtual sp<IMemoryHeap> getRawHeap() const;

    virtual void        setCallbacks(notify_callback notify_cb,
                                     data_callback data_cb,
                                     data_callback_timestamp data_cb_timestamp,
                                     void* user);

    virtual void        enableMsgType(int32_t msgType);
    virtual void        disableMsgType(int32_t msgType);
    virtual bool        msgTypeEnabled(int32_t msgType);
//[amba_overlay
//    virtual status_t	setPreviewPosition(int32_t vout_id, int32_t x, int32_t y, int32_t width, int32_t height);
virtual bool useOverlay();
virtual status_t	setOverlay(const sp<Overlay> &overlay);
//]amba_overlay
    virtual status_t    startPreview();
    virtual void        stopPreview();
    virtual bool        previewEnabled();

    virtual status_t    startRecording();
    virtual void        stopRecording();
    virtual bool        recordingEnabled();
    virtual void        releaseRecordingFrame(const sp<IMemory>& mem);

    virtual status_t    autoFocus();
    virtual status_t    cancelAutoFocus();
    virtual status_t    takePicture();
    virtual status_t    cancelPicture();
    virtual status_t    dump(int fd, const Vector<String16>& args) const;
    virtual status_t    setParameters(const CameraParameters& params);
    virtual CameraParameters  getParameters() const;
    virtual status_t    sendCommand(int32_t command, int32_t arg1,
                                    int32_t arg2);
    virtual void release();

    static sp<CameraHardwareInterface> createInstance();

private:

    AMCameraHal();
    virtual ~AMCameraHal();
	
    static wp<CameraHardwareInterface> singleton;

    static const int kBufferCount = 4;

    class PreviewThread : public Thread {
        AMCameraHal* mHardware;
    public:
        PreviewThread(AMCameraHal* hw)
            : Thread(false), mHardware(hw) { }
        virtual void onFirstRef() {
            run("CameraPreviewThread", PRIORITY_URGENT_DISPLAY);
        }
        virtual bool threadLoop() {
            mHardware->previewThread();
            // loop until we need to quit
            return true;
        }
    };

    class RecordingThread : public Thread {
        AMCameraHal* mHardware;
    public:
        RecordingThread(AMCameraHal* hw)
            : Thread(false), mHardware(hw) { }

        virtual bool threadLoop() {
            mHardware->recordingThread();
            return false;
        }
    };


    void initDefaultParameters();
    void initHeapLocked();

    int previewThread();

    static int beginAutoFocusThread(void *cookie);
    int autoFocusThread();

    mutable Mutex       mLock;

    CameraParameters    mParameters;

    sp<MemoryHeapBase>  mPreviewHeap;
    sp<MemoryHeapBase>  mRawHeap;
    sp<MemoryBase>      mBuffers[kBufferCount];

    bool                mPreviewRunning;
    int                 mPreviewFrameSize;

    // protected by mLock
    sp<PreviewThread>   mPreviewThread;

    notify_callback    mNotifyCb;
    data_callback      mDataCb;
    data_callback_timestamp mDataCbTimestamp;
    void               *mCallbackCookie;

    int32_t             mMsgEnabled;

    // only used from PreviewThread
    int                 mCurrentPreviewFrame;

    sp<RecordingThread>  mRecordingThread;
    int mRecordingFrameSize;
    data_callback_timestamp mRecordingCallback;
    static const int   videoBufferCount = VIDEO_BUF_COUNT;
    static const int   SPS_PPS_SIZE = 256;
    int m_sps_pps_len;
    sp<MemoryHeapBase>  mVideoHeap;
    sp<AmbaMemory> mVideoBuffer[videoBufferCount+1]; //the last ambamemory is to store sps/pps information
    int  mVideoBufferUsing[videoBufferCount + 1];
    unsigned char mAmbaInfo[64];
    int mInfoCnt;
    status_t writeStreamBe(int code, int bytes); //write code to stream in Big Endian
    static int get_sps_pps(CSPECIFIC_INFO *av);

    void recordingThread();
    int set_encode_format();
	AM_IAV *mpIAV;

    unsigned char *bsb_mem;
    int bsb_size;
    sp<BSBMemoryHeap> mBSBHeap;

};

}; //namespace android
#endif
