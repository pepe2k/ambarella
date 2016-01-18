
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(AMF_TOP)/include
LOCAL_C_INCLUDES += $(FFMPEG_TOP)
LOCAL_C_INCLUDES += $(AMF_TOP)/android
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../frameworks/base/media/libmediaplayerservice

include $(AMF_TOP)/build/core/buildflags.mk
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)

LOCAL_SRC_FILES := pbtest.cpp
LOCAL_STATIC_LIBRARIES := libAMF
LOCAL_MODULE := pbtest
ifeq ($(AM_PBTEST_DEBUG), true)
LOCAL_STATIC_LIBRARIES += libavformat libavcodec libavutil
else
LOCAL_SHARED_LIBRARIES += libavformat libavcodec libavutil
endif
LOCAL_STATIC_LIBRARIES += libAMBA_avcodec libvpx libpv_amr_nb_wrapper libpv_amr_wb_wrapper libpv_mp3_wrapper liblog libz libft2 libaacdec libaacenc

LOCAL_SHARED_LIBRARIES += libmedia libmediaplayerservice #libvout
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(AMF_TOP)/include
LOCAL_C_INCLUDES += $(AMF_TOP)/imgproc
LOCAL_C_INCLUDES += $(AMF_TOP)/imgproc/vout
LOCAL_C_INCLUDES += $(AMF_TOP)/imgproc/vin
LOCAL_C_INCLUDES += $(FFMPEG_TOP)
LOCAL_C_INCLUDES += $(AMF_TOP)/android
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../frameworks/base/media/libmediaplayerservice

LOCAL_C_INCLUDES += $(AMF_TOP)/../external/LDWS
LOCAL_C_INCLUDES += $(AMF_TOP)/../external/LDWS/ivLDWSEngine

include $(AMF_TOP)/build/core/buildflags.mk
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)
LOCAL_CFLAGS += -D_LINUX -DUSE_IVCV -DIVLDWSENGINE_EXPORTS

LOCAL_SRC_FILES := rectest.cpp
LOCAL_STATIC_LIBRARIES := libAMF

#TARGET_STRIP_MODULE := false
LOCAL_MODULE := rectest
ifeq ($(AM_PBTEST_DEBUG), true)
LOCAL_STATIC_LIBRARIES += libavformat libavcodec libavutil
else
LOCAL_SHARED_LIBRARIES += libavformat libavcodec libavutil
endif
LOCAL_STATIC_LIBRARIES += libAMBA_avcodec libvpx libpv_amr_nb_wrapper libpv_amr_wb_wrapper libpv_mp3_wrapper liblog libz libft2 libaacdec libaacenc
LOCAL_STATIC_LIBRARIES += libamldws libivldws

ifeq ($(strip $(PLATFORM_ANDROID_ITRON)), true)
LOCAL_SHARED_LIBRARIES += libaic libimq liblumq libambastream libmqueue
endif
#TARGET_STRIP_MODULE := false
LOCAL_SHARED_LIBRARIES += libmedia libmediaplayerservice #libvout
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(AMF_TOP)/include
LOCAL_C_INCLUDES += $(AMF_TOP)/imgproc
LOCAL_C_INCLUDES += $(AMF_TOP)/imgproc/vout
LOCAL_C_INCLUDES += $(AMF_TOP)/imgproc/vin
LOCAL_C_INCLUDES += $(FFMPEG_TOP)
LOCAL_C_INCLUDES += $(AMF_TOP)/android
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../frameworks/base/media/libmediaplayerservice

include $(AMF_TOP)/build/core/buildflags.mk
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)

LOCAL_SRC_FILES := dutest.cpp
LOCAL_STATIC_LIBRARIES := libAMF
#TARGET_STRIP_MODULE := false
LOCAL_MODULE := dutest
ifeq ($(AM_PBTEST_DEBUG), true)
LOCAL_STATIC_LIBRARIES += libavformat libavcodec libavutil
else
LOCAL_SHARED_LIBRARIES += libavformat libavcodec libavutil
endif
LOCAL_STATIC_LIBRARIES += libAMBA_avcodec libvpx libpv_amr_nb_wrapper libpv_amr_wb_wrapper libpv_mp3_wrapper liblog libz libft2 libaacdec libaacenc

ifeq ($(strip $(PLATFORM_ANDROID_ITRON)), true)
LOCAL_SHARED_LIBRARIES += libaic libimq liblumq libambastream libmqueue
endif
#TARGET_STRIP_MODULE := false
LOCAL_SHARED_LIBRARIES += libmedia libmediaplayerservice #libvout
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(AMF_TOP)/include
LOCAL_C_INCLUDES += $(FFMPEG_TOP)
LOCAL_C_INCLUDES += $(AMF_TOP)/android
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../frameworks/base/media/libmediaplayerservice

include $(AMF_TOP)/build/core/buildflags.mk
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)

LOCAL_SRC_FILES := test_2udec.cpp
LOCAL_STATIC_LIBRARIES := libAMF
LOCAL_MODULE := test_2udec
ifeq ($(AM_PBTEST_DEBUG), true)
LOCAL_STATIC_LIBRARIES += libavformat libavcodec libavutil
else
LOCAL_SHARED_LIBRARIES += libavformat libavcodec libavutil
endif
LOCAL_STATIC_LIBRARIES += libpv_amr_nb_wrapper libpv_amr_wb_wrapper libpv_mp3_wrapper libAMBA_avcodec liblog libz libft2

LOCAL_SHARED_LIBRARIES += libmedia
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(AMF_TOP)/include
LOCAL_C_INCLUDES += $(FFMPEG_TOP)
LOCAL_C_INCLUDES += $(AMF_TOP)/android
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../frameworks/base/media/libmediaplayerservice

LOCAL_C_INCLUDES += $(AMF_TOP)/../../../../ambarella
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../../ambarella/build/include
LOCAL_C_INCLUDES += $(AMF_TOP)/src/filters

include $(AMF_TOP)/build/core/buildflags.mk
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)

LOCAL_SRC_FILES := test_vepreview.cpp
LOCAL_STATIC_LIBRARIES := libAMF
LOCAL_MODULE := test_vepreview
ifeq ($(AM_PBTEST_DEBUG), true)
LOCAL_STATIC_LIBRARIES += libavformat libavcodec libavutil
else
LOCAL_SHARED_LIBRARIES += libavformat libavcodec libavutil
endif
LOCAL_STATIC_LIBRARIES += libpv_amr_nb_wrapper libpv_amr_wb_wrapper libpv_mp3_wrapper libAMBA_avcodec liblog libz libft2
LOCAL_SHARED_LIBRARIES += libutils libEGL libGLESv1_CM libui

LOCAL_SHARED_LIBRARIES += libmedia
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(AMF_TOP)/include
LOCAL_C_INCLUDES += $(FFMPEG_TOP)
LOCAL_C_INCLUDES += $(AMF_TOP)/android
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../frameworks/base/media/libmediaplayerservice

LOCAL_C_INCLUDES += $(AMF_TOP)/../../../../ambarella
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../../ambarella/build/include
LOCAL_C_INCLUDES += $(AMF_TOP)/src/filters

include $(AMF_TOP)/build/core/buildflags.mk
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)

LOCAL_SRC_FILES := test_transcode.cpp
LOCAL_STATIC_LIBRARIES := libAMF
LOCAL_MODULE := test_transcode

ifeq ($(AM_PBTEST_DEBUG), true)
LOCAL_STATIC_LIBRARIES += libavformat libavcodec libavutil
else
LOCAL_SHARED_LIBRARIES += libavformat libavcodec libavutil
endif
LOCAL_STATIC_LIBRARIES += libpv_amr_nb_wrapper libpv_amr_wb_wrapper libpv_mp3_wrapper libAMBA_avcodec liblog libz libft2

LOCAL_SHARED_LIBRARIES += libmedia
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(AMF_TOP)/include
LOCAL_C_INCLUDES += $(FFMPEG_TOP)
LOCAL_C_INCLUDES += $(AMF_TOP)/android
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../frameworks/base/media/libmediaplayerservice

LOCAL_C_INCLUDES += $(AMF_TOP)/../../../../ambarella
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../../ambarella/build/include
LOCAL_C_INCLUDES += $(AMF_TOP)/src/filters
LOCAL_C_INCLUDES += $(AMF_TOP)/src/filters/general
LOCAL_C_INCLUDES += $(AMF_TOP)/src/framework
LOCAL_C_INCLUDES += $(AMF_TOP)/src/engines
LOCAL_C_INCLUDES += $(AMF_TOP)/src/audio

include $(AMF_TOP)/build/core/buildflags.mk
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)

LOCAL_SRC_FILES := mdectest.cpp
LOCAL_STATIC_LIBRARIES := libAMF
LOCAL_MODULE := mdectest

ifeq ($(AM_PBTEST_DEBUG), true)
LOCAL_STATIC_LIBRARIES += libavformat libavcodec libavutil
else
LOCAL_SHARED_LIBRARIES += libavformat libavcodec libavutil
endif
LOCAL_STATIC_LIBRARIES += libpv_amr_nb_wrapper libpv_amr_wb_wrapper libpv_mp3_wrapper libAMBA_avcodec liblog libz libft2 libjpegenc libaacdec

LOCAL_SHARED_LIBRARIES += libmedia libmediaplayerservice
include $(BUILD_EXECUTABLE)

ifneq ($(strip $(PLATFORM_ANDROID_ITRON)),true)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(AMF_TOP)/include
LOCAL_C_INCLUDES += $(AMF_TOP)/imgproc
LOCAL_C_INCLUDES += $(AMF_TOP)/imgproc/vout
LOCAL_C_INCLUDES += $(AMF_TOP)/imgproc/vin
include $(AMF_TOP)/build/core/buildflags.mk
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)

LOCAL_SRC_FILES := vt.cpp
LOCAL_STATIC_LIBRARIES := libAMFrec_pretreat libAMFvout_control  libAMFvin_control libimg_algo
LOCAL_SHARED_LIBRARIES := libdbus liblog
#TARGET_STRIP_MODULE := false
LOCAL_MODULE := vt
include $(BUILD_EXECUTABLE)
endif

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(AMF_TOP)/include
LOCAL_C_INCLUDES += $(AMF_TOP)/android

LOCAL_STATIC_LIBRARIES := libAMF
LOCAL_SHARED_LIBRARIES += libcutils libutils

include $(AMF_TOP)/build/core/buildflags.mk
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)

LOCAL_SRC_FILES := ../android/TSFileCombiner.cpp test_tscombine.cpp
LOCAL_MODULE := test_tscombine
include $(BUILD_EXECUTABLE)

PRODUCT_COPY_FILES += $(AMF_TOP)/test/autoplay_android:system/bin/autoplay

