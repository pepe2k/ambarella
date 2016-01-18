
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
	dequantize_arm.c \
	reconintra_arm.c \
	yv12extend_arm.c \
	dequantdcidct_neon.s \
	dequantidct_neon.s \
	dequantizeb_neon.s \
	shortidct4x4llm_neon.s \
	shortidct4x4llm_1_neon.s \
	iwalsh_neon.s \
	buildintrapredictorsmby_neon.s \
	copymem8x4_neon.s \
	copymem8x8_neon.s \
	copymem16x16_neon.s \
	reconb_neon.s \
	recon2b_neon.s \
	recon4b_neon.s \
	vp8_vpxyv12_copyframe_func_neon.s \
	vp8_vpxyv12_copyframeyonly_neon.s \
	vp8_vpxyv12_extendframeborders_neon.s


LOCAL_MODULE := libvpx_neon
include $(BUILD_STATIC_LIBRARY)
