
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(AMF_TOP)/include $(AMF_TOP)/src/engines $(AMF_TOP)/src/audio
LOCAL_C_INCLUDES += $(AMF_TOP)/src/filters/general $(AMF_TOP)/src/framework
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../../ambarella/build/include
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../frameworks/base/media/libmediaplayerservice
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../hardware/ambarella/libvout
LOCAL_C_INCLUDES += $(FFMPEG_TOP)


include $(AMF_TOP)/build/core/buildflags.mk
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)

LOCAL_SRC_FILES := amba_dsp_common.cpp

ifeq ($(TARGET_USE_AMBARELLA_I1_DSP), true)
LOCAL_SRC_FILES += amba_dsp_ione.cpp
endif
ifeq ($(TARGET_USE_AMBARELLA_A5S_DSP), true)
LOCAL_SRC_FILES += amba_dsp_a5s.cpp
endif

LOCAL_SHARED_LIBRARIES := libutils liblog

LOCAL_MODULE := libAMF_dsp
include $(BUILD_STATIC_LIBRARY)
