LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../../ambarella/build/include

LOCAL_C_INCLUDES += $(FFMPEG_TOP)/libavcodec
LOCAL_C_INCLUDES += $(FFMPEG_TOP)/libavformat
LOCAL_C_INCLUDES += $(FFMPEG_TOP)

LOCAL_C_INCLUDES += $(AMF_TOP)/include $(AMF_TOP)/src/engines
LOCAL_C_INCLUDES += $(AMF_TOP)/include $(AMF_TOP)/src/dsp
LOCAL_C_INCLUDES += $(AMF_TOP)/include $(AMF_TOP)/src/audio
LOCAL_C_INCLUDES += $(AMF_TOP)/include $(AMF_TOP)/src/filters
LOCAL_C_INCLUDES += $(AMF_TOP)/include $(AMF_TOP)/src/filters/general/misc
LOCAL_C_INCLUDES += $(AMF_TOP)/include $(AMF_TOP)/src/framework
LOCAL_C_INCLUDES += $(AMF_TOP)/libaac/include

#LOCAL_C_INCLUDES += $(AMF_TOP)/../external/coreavc/include

include $(AMF_TOP)/build/core/buildflags.mk
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)

LOCAL_SRC_FILES := \
	general_demuxer_filter.cpp \
	general_decoder_filter.cpp \
	general_renderer_filter.cpp \
	general_transcoder_filter.cpp \
	general_transc_audiosink_filter.cpp \
	g_render_out.cpp \
	general_parse.cpp \
	general_mw.cpp \
	general_header.cpp \
	g_ffmpeg_decoder.cpp \
	g_sync_renderer.cpp \
	g_ffmpeg_demuxer_exqueue.cpp \
	g_ffmpeg_demuxer_exnet.cpp \
	gvideo_decoder_dsp_exqueue.cpp \
	general_audio_manager.cpp \
	g_ffmpeg_video_decoder.cpp

#LOCAL_SRC_FILES += g_coreavc_decoder.cpp

LOCAL_WHOLE_STATIC_LIBRARIES := libGMF_misc

LOCAL_SHARED_LIBRARIES := libutils liblog

LOCAL_MODULE := libGMF
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)
