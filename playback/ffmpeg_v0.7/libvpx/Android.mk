
#LIBVPX_TOP := $(call my-dir)

#include $(LIBVPX_TOP)/vp8/common/Android.mk
#include $(LIBVPX_TOP)/vp8/decoder/Android.mk
#include $(LIBVPX_TOP)/vpx_codec/src/Android.mk
#include $(LIBVPX_TOP)/vpx_mem/Android.mk
#include $(LIBVPX_TOP)/vpx_scale/generic/Android.mk
#include $(LIBVPX_TOP)/vp8/decoder/arm/neon/Android.mk

#include $(CLEAR_VARS)
#LOCAL_MODULE_TAGS := optional
#LOCAL_WHOLE_STATIC_LIBRARIES := \
#	libvpx_common \
#	libvpx_decoder \
#	libvpx_vpx_codec \
#	libvpx_vpx_mem \
#	libvpx_vpx_scale \
#	libvpx_neon

#LOCAL_MODULE := libvpx
#include $(BUILD_STATIC_LIBRARY)

#include $(LIBVPX_TOP)/test/Android.mk

