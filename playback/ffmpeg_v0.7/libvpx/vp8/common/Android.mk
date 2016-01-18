
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
	alloccommon.c \
	blockd.c \
	debugmodes.c \
	entropy.c \
	entropymode.c \
	entropymv.c \
	extend.c \
	filter_c.c \
	findnearmv.c \
	idctllm.c \
	invtrans.c \
	loopfilter.c \
	loopfilter_filters.c \
	mbpitch.c \
	modecont.c \
	modecontext.c \
	postproc.c \
	predictdc.c \
	quant_common.c \
	recon.c \
	reconinter.c \
	reconintra4x4.c \
	reconintra.c \
	segmentation_common.c \
	setupintrarecon.c \
	swapyv12buffer.c \
	textblit.c \
	treecoder.c \
	systemdependent.c
	
	

LOCAL_MODULE := libvpx_common
include $(BUILD_STATIC_LIBRARY)
