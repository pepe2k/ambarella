LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/../av.mk
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(FFFILES)

LOCAL_C_INCLUDES :=		\
	$(LOCAL_PATH)		\
	system/core/include	\
	$(LOCAL_PATH)/..	\
	$(ConfigFilesPath)

LOCAL_CFLAGS += $(FFCFLAGS)

#LOCAL_CFLAGS += -mfpu=neon -mfloat-abi=softfp \
#	-ftree-vectorize -ffast-math -mvectorize-with-neon-quad

LOCAL_SHARED_LIBRARIES := libutils liblog
LOCAL_SHARED_LIBRARIES := $(FFLIBS)
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE := $(FFNAME)

ifeq ($(AM_PBTEST_DEBUG), true)
include $(BUILD_STATIC_LIBRARY)
else
include $(BUILD_SHARED_LIBRARY)
endif