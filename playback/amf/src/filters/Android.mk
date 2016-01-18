
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(AMF_TOP)/include $(AMF_TOP)/src/engines
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../../ambarella/build/include
LOCAL_C_INCLUDES += $(AMF_TOP)/libaac/include
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../frameworks/base/media/libmediaplayerservice
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../hardware/ambarella/libvout
LOCAL_C_INCLUDES += $(FFMPEG_TOP)/libavcodec
LOCAL_C_INCLUDES += $(FFMPEG_TOP)/libavformat
LOCAL_C_INCLUDES += $(FFMPEG_TOP)
LOCAL_C_INCLUDES += $(AMF_TOP)/extern/amaudio/include
LOCAL_C_INCLUDES += $(AMF_TOP)/src/dsp
LOCAL_C_INCLUDES += $(AMF_TOP)/../codec/asm/rv40
LOCAL_C_INCLUDES += external/freetype/include
LOCAL_C_INCLUDES += external/include/freetype2
LOCAL_C_INCLUDES += $(AMF_TOP)/src/audio
#LOCAL_C_INCLUDES += $(AMF_TOP)/../../../../ambarella/prebuild/third-party/misc/include
include $(AMF_TOP)/build/core/buildflags.mk
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)

ifeq ($(strip $(BOARD_USE_CAMERA2)), true)
LOCAL_CFLAGS += -DENABLE_AMBA_AAC=false
endif

ifeq ($(strip $(PLATFORM_ANDROID_ITRON)), true)
LOCAL_C_INCLUDES += $(AMF_TOP)/../mqueue
LOCAL_C_INCLUDES += $(AMF_TOP)/../aic
endif

LOCAL_SRC_FILES := \
	audio_decoder_hw.cpp \
	amba_video_sink.cpp \
	am_hwtimer.cpp \
	amdsp_common.cpp \
	amba_encoder.cpp \
	simple_muxer.cpp \
	mp4_muxer.cpp \
	video_encoder.cpp \
	ffmpeg_muxer.cpp \
	ffmpeg_muxer2.cpp \
	audio_input.cpp \
	ffmpeg_decoder.cpp \
	video_renderer.cpp \
	audio_renderer.cpp \
	audio_muxer.cpp \
	simple_filesave.cpp \
	amba_audio_decoder.cpp \
	amba_video_decoder.cpp \
	audio_effecter.cpp \
	amba_video_encoder.cpp \
	amba_audio_input.cpp \
	amba_create_mp4.cpp \
	subtitle_renderer.cpp \
	audio_input2.cpp \
	aac_encoder.cpp \
	ffmpeg_encoder.cpp \
	video_effecter_preview.cpp\
	video_transcoder.cpp\
	video_mem_encoder.cpp\
	pridata_composer.cpp\
	pridata_parser.cpp\
	ffmpeg_util.cpp\
	rtsp_demuxer.cpp\
	am_ffmpeg.cpp

LOCAL_SRC_FILES += \
	 video_decoder_dsp_ve.cpp general_decode_filter_ve.cpp

# Choose ffmpeg_demux for prja
ifeq ($(AMF_AOF_PJ203_CONFIG), true)
LOCAL_SRC_FILES += ffmpeg_demuxer_prja.cpp
else
LOCAL_SRC_FILES += ffmpeg_demuxer.cpp
endif

#ifeq ($(strip $(AM_DEBUG_NEW_FILTER)), false)
#LOCAL_SRC_FILES += \
#	video_encoder.cpp
#else
#LOCAL_SRC_FILES += \
#	video_encoder_iav.cpp
#endif

LOCAL_SHARED_LIBRARIES := libutils liblog
LOCAL_STATIC_LIBRARIES := libaacdec
ifeq ($(strip $(PLATFORM_ANDROID_ITRON)), true)
LOCAL_SRC_FILES += itron_encoder.cpp
LOCAL_SHARED_LIBRARIES += libaic libimq liblumq libambastream libmqueue
endif


LOCAL_MODULE := libAMF_filters
include $(BUILD_STATIC_LIBRARY)

#include $(AMF_TOP)/src/filters/rtsp_filter/Android.mk


