
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(AMF_TOP)/include
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../../ambarella/build/include
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../../ambarella

include $(AMF_TOP)/build/core/buildflags.mk
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)

LOCAL_SRC_FILES := \
	am_camera.cpp \
	am_util.cpp \
	am_net.cpp \
	misc_guids.cpp \
	am_pridata.cpp
	
LOCAL_SHARED_LIBRARIES := libutils liblog

LOCAL_MODULE := libAMF_misc
include $(BUILD_STATIC_LIBRARY)
