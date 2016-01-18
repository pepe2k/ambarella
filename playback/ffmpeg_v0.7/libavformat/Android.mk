LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/../av.mk
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(FFFILES)

LOCAL_C_INCLUDES :=		\
	$(LOCAL_PATH)		\
	$(LOCAL_PATH)/..	\
	system/core/include	\
	external/zlib		\
	$(ConfigFilesPath)

LOCAL_CFLAGS += $(FFCFLAGS)

#compile option for FILE-IO, to use DIRECT_IO mode
#LOCAL_CFLAGS += -DAMF_RECORD_DIRECTIO

LOCAL_CFLAGS += -include "string.h" -Dipv6mr_interface=ipv6mr_ifindex

LOCAL_SHARED_LIBRARIES += libutils liblog

ifeq ($(AM_PBTEST_DEBUG), true)
LOCAL_STATIC_LIBRARIES += libavcodec libavutil libz
else
LOCAL_SHARED_LIBRARIES += libavcodec libavutil libz
endif

LOCAL_MODULE := $(FFNAME)
LOCAL_PRELINK_MODULE := false

ifeq ($(AM_PBTEST_DEBUG), true)
include $(BUILD_STATIC_LIBRARY)
else
include $(BUILD_SHARED_LIBRARY)
endif
