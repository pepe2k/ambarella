
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(AMF_TOP)/include

include $(AMF_TOP)/build/core/buildflags.mk
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)

LOCAL_SRC_FILES := \
	osal_linux.cpp \
	am_queue.cpp \
	msgsys.cpp \
	am_base.cpp \
	am_if.cpp \
	am_futif.cpp \
	am_sink.cpp \
	am_param.cpp

LOCAL_SHARED_LIBRARIES := libutils liblog

LOCAL_MODULE := libAMF_framework
include $(BUILD_STATIC_LIBRARY)
