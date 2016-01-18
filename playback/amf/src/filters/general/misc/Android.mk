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
LOCAL_C_INCLUDES += $(AMF_TOP)/include $(AMF_TOP)/src/filters/general
LOCAL_C_INCLUDES += $(AMF_TOP)/include $(AMF_TOP)/src/filters/general/misc
LOCAL_C_INCLUDES += $(AMF_TOP)/include $(AMF_TOP)/src/framework

include $(AMF_TOP)/build/core/buildflags.mk
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)

LOCAL_SRC_FILES := \
	general_muxer_save.cpp \
	gmf_osd_shower.cpp \
	g_muxer_ffmpeg.cpp \
	g_simple_save.cpp \
	general_layout_manager.cpp \
	general_pipeline.cpp


LOCAL_SHARED_LIBRARIES := libutils liblog

LOCAL_MODULE := libGMF_misc
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)

