
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(LIBVPX_TOP)
LOCAL_C_INCLUDES += $(LIBVPX_TOP)/vp8
LOCAL_C_INCLUDES += $(LIBVPX_TOP)/test
LOCAL_C_INCLUDES += $(LIBVPX_TOP)/vp8/common
LOCAL_C_INCLUDES += $(LIBVPX_TOP)/vp8/decoder
LOCAL_C_INCLUDES += $(LIBVPX_TOP)/vpx_codec
LOCAL_C_INCLUDES += $(LIBVPX_TOP)/vpx_codec/internal
LOCAL_C_INCLUDES += $(LIBVPX_TOP)/vpx_mem
LOCAL_C_INCLUDES += $(LIBVPX_TOP)/vpx_mem/include
LOCAL_C_INCLUDES += $(LIBVPX_TOP)/vpx_ports
LOCAL_C_INCLUDES += $(LIBVPX_TOP)/vpx_scale/include
LOCAL_C_INCLUDES += $(LIBVPX_TOP)/vpx_scale/include/generic
LOCAL_C_INCLUDES += $(LIBVPX_TOP)/vpx_scale

LOCAL_SRC_FILES := simple_decoder.c

LOCAL_STATIC_LIBRARIES := libvpx

LOCAL_MODULE := vp8dec

include $(BUILD_EXECUTABLE)



