##
## camera/modules/stream/output/make.inc
##
## History:
##    2012/05/30 - [Yupeng Chang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

include $(call all-subdir-makefiles)

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
OUTPUT_VER_FILE   = $(CAMERA_DIR)/include/stream/version.h
OUTPUT_VER_PREFIX = STREAM

OUTPUT_TARGET  = libamstreamoutput.so
LOCAL_TARGET := $(OUTPUT_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/base/*.cpp \
                           $(LOCAL_PATH)/muxer/ts/*.cpp \
                           $(LOCAL_PATH)/muxer/mp4/*.cpp \
                           $(LOCAL_PATH)/muxer/rtsp/*.cpp \
                           $(LOCAL_PATH)/muxer/jpeg/*.cpp \
                           $(LOCAL_PATH)/muxer/raw/*.cpp \
                           $(LOCAL_PATH)/player/base/*.cpp \
                           $(LOCAL_PATH)/player/audio/*.cpp \
                           $(LOCAL_PATH)/sink/*.cpp)
LOCAL_CFLAGS := -I$(LOCAL_PATH)/base \
                -I$(LOCAL_PATH)/muxer/include \
                -I$(LOCAL_PATH)/muxer/ts \
                -I$(LOCAL_PATH)/muxer/mp4 \
                -I$(LOCAL_PATH)/muxer/rtsp \
                -I$(LOCAL_PATH)/player/base \
                -I$(LOCAL_PATH)/player/audio \
                -I$(LOCAL_PATH)/sink \
                -I$(LOCAL_PATH)/include \
                -I$(CAMERA_DIR)/include \
                -I$(CAMERA_DIR)/modules/stream/include  \
                -I$(CAMERA_DIR)/modules/stream/amf/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/libcurl/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/openssl/include \
                -Werror

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

OUTPUT_SO_VER_MAJOR  := $(shell awk '/define\ $(OUTPUT_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(OUTPUT_VER_FILE))
OUTPUT_SO_VER_MINOR  := $(shell awk '/define\ $(OUTPUT_VER_PREFIX)_LIB_MINOR/{print $$3}' $(OUTPUT_VER_FILE))
OUTPUT_SO_VER_PATCH  := $(shell awk '/define\ $(OUTPUT_VER_PREFIX)_LIB_PATCH/{print $$3}' $(OUTPUT_VER_FILE))
OUTPUT_SO_VER_STRING := $(OUTPUT_SO_VER_MAJOR).$(OUTPUT_SO_VER_MINOR).$(OUTPUT_SO_VER_PATCH)

OUTPUT_SO_NAME   = $(patsubst %.so,%.so.$(OUTPUT_SO_VER_MAJOR), $(OUTPUT_TARGET))
OUTPUT_SO_TARGET = $(patsubst %.so,%.so.$(OUTPUT_SO_VER_STRING), $(OUTPUT_TARGET))

LOCAL_SO_NAME := $(OUTPUT_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(OUTPUT_SO_TARGET)
	@ln -sf $(OUTPUT_SO_TARGET) $(CAMERA_LIB_DIR)/$(OUTPUT_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

