LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/../av.mk
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(FFFILES) \
        amba_dec_dsputil.c
#ifeq ($(HAVE_NEON), yes)
#LOCAL_SRC_FILES += amba_dec_dsputil.c
#endif


LOCAL_C_INCLUDES :=		\
	$(LOCAL_PATH)		\
	$(LOCAL_PATH)/..	\
	system/core/include	\
	external/zlib		\
	$(ConfigFilesPath)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvpx
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvpx/vp8
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvpx/test
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvpx/vp8/common
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvpx/vp8/decoder
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvpx/vpx_codec
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvpx/vpx_codec/internal
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvpx/vpx_mem
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvpx/vpx_mem/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvpx/vpx_ports
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvpx/vpx_scale/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvpx/vpx_scale/include/generic
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvpx/vpx_scale
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../frameworks/base/media/libstagefright/wrapper

LOCAL_CFLAGS += $(FFCFLAGS)

#LOCAL_CFLAGS += -mfpu=neon -mfloat-abi=softfp \
#	-ftree-vectorize -ffast-math -mvectorize-with-neon-quad
#LOCAL_PRELINK_MODULE := false

LOCAL_SHARED_LIBRARIES := libutils liblog
LOCAL_STATIC_LIBRARIES := libvpx libpv_amr_nb_wrapper libpv_amr_wb_wrapper libpv_mp3_wrapper libAMBA_avcodec

ifeq ($(AM_PBTEST_DEBUG), true)
LOCAL_STATIC_LIBRARIES += libavutil libz
else
LOCAL_SHARED_LIBRARIES += libavutil libz
endif

LOCAL_MODULE := $(FFNAME)
LOCAL_PRELINK_MODULE := false

ifeq ($(AM_PBTEST_DEBUG), true)
include $(BUILD_STATIC_LIBRARY)
else
include $(BUILD_SHARED_LIBRARY)
endif
