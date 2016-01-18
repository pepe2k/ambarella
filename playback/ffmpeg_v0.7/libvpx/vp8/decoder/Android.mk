
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(LIBVPX_TOP)
LOCAL_C_INCLUDES += $(LIBVPX_TOP)/vp8
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

LOCAL_SRC_FILES := \
	dboolhuff.c \
	decodemv.c \
	decodframe.c \
	demode.c \
	dequantize.c \
	detokenize.c \
	dsystemdependent.c \
	onyxd_if.c \
	threading.c \
	vp8_dx_iface.c
	
	

LOCAL_MODULE := libvpx_decoder
include $(BUILD_STATIC_LIBRARY)
