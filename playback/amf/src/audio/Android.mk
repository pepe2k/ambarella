
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(AMF_TOP)/include $(AMF_TOP)/src/engines
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../../ambarella/build/include
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../frameworks/base/media/libmediaplayerservice
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../hardware/ambarella/libvout
LOCAL_C_INCLUDES += $(FFMPEG_TOP)


include $(AMF_TOP)/build/core/buildflags.mk
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)


LOCAL_SRC_FILES := \
	audio_if.cpp \
	audio_android.cpp

LOCAL_SHARED_LIBRARIES := libutils liblog

LOCAL_MODULE := libAMF_audio
include $(BUILD_STATIC_LIBRARY)

