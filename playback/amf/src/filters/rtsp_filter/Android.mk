LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES += $(AMF_TOP)/include $(AMF_TOP)/src/engines
include $(AMF_TOP)/build/core/buildflags.mk
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)

LOCAL_SRC_FILES := \
	adts_rtp_sink.cpp \
	adts_server_media_subsession.cpp \
	h264_rtp_sink.cpp \
	h264_server_media_subsession.cpp \
	helper.cpp \
	random.cpp \
	rtp_sink.cpp \
	rtsp_client_session.cpp \
	rtsp_filter.cpp \
	server_media_session.cpp \
	sock_helper.cpp


LOCAL_MODULE := librtspfilter
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)

