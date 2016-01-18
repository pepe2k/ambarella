
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(AMF_TOP)/include $(AMF_TOP)/framework $(AMF_TOP)/src/filters $(AMF_TOP)/src/audio
LOCAL_C_INCLUDES += $(AMF_TOP)/src/filters/general $(AMF_TOP)/src/filters/general/misc $(AMF_TOP)/src/framework
LOCAL_C_INCLUDES += $(FFMPEG_TOP)/libavcodec
LOCAL_C_INCLUDES += $(FFMPEG_TOP)/libavformat
LOCAL_C_INCLUDES += $(FFMPEG_TOP)
LOCAL_C_INCLUDES += $(AMF_TOP)/../../../../ambarella/build/include
LOCAL_C_INCLUDES += $(AMF_TOP)/imgproc
LOCAL_C_INCLUDES += $(AMF_TOP)/imgproc/vout

include $(AMF_TOP)/build/core/buildflags.mk
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)

ifeq ($(strip $(BOARD_USE_CAMERA2)), true)
LOCAL_CFLAGS += -DENABLE_AMBA_MP4=false
endif

ifeq ($(strip $(BOARD_ENABLE_OSD_ON_HDMI)), true)
LOCAL_CFLAGS += -DENABLE_OSD_ON_HDMI
endif

LOCAL_SRC_FILES := \
	active_pb_engine.cpp \
	pb_engine.cpp \
	filter_registry.cpp \
	filter_list.cpp \
	pbif.cpp \
	active_record_engine.cpp \
	active_duplex_engine.cpp \
	engine_guids.cpp \
	record_if.cpp \
	am_record_if.cpp \
	active_ve_engine.cpp \
	ve_if.cpp \
	active_mdec_engine.cpp \
	mdec_if.cpp \
	streamming_if.cpp \
	streamming_server.cpp\
	random.cpp\
	ws_discovery.cpp\
	ws_discovery_impl.cpp \
	motiondetectreceiver.cpp

LOCAL_SHARED_LIBRARIES := libutils liblog

LOCAL_MODULE := libAMF_engines
include $(BUILD_STATIC_LIBRARY)
