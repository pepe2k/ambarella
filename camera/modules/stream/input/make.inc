##
## camera/modules/stream/input/make.inc
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

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
INPUT_VER_FILE   = $(CAMERA_DIR)/include/stream/version.h
INPUT_VER_PREFIX = STREAM

INPUT_TARGET  = libamstreaminput.so
LOCAL_TARGET := $(INPUT_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/base/*.cpp \
                $(LOCAL_PATH)/audio/source/*.cpp \
                $(LOCAL_PATH)/audio/codec/*.cpp \
                $(LOCAL_PATH)/video/*.cpp \
                $(LOCAL_PATH)/sei/*.cpp \
                $(LOCAL_PATH)/event/*.cpp \
                $(LOCAL_PATH)/demuxer/base/*.cpp)
LOCAL_CFLAGS := -I$(LOCAL_PATH)/base \
                -I$(LOCAL_PATH)/audio/source \
                -I$(LOCAL_PATH)/audio/codec \
                -I$(LOCAL_PATH)/video \
                -I$(LOCAL_PATH)/sei \
                -I$(LOCAL_PATH)/event \
                -I$(LOCAL_PATH)/demuxer/base \
                -I$(LOCAL_PATH)/demuxer/codec \
                -I$(LOCAL_PATH)/include \
                -I$(CAMERA_DIR)/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/aac/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/libg726/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/opus/include \
                -I$(CAMERA_DIR)/modules/stream/amf/include \
                -I$(CAMERA_DIR)/modules/stream/include \
                -Werror

ifeq ($(CONFIG_ARCH_A5S), y)
ifeq ($(ENABLE_FRAME_STATISTICS), y)
LOCAL_CFLAGS += -I$(AMB_TOPDIR)/packages/adaptive_rc/include
endif
endif

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

INPUT_SO_VER_MAJOR  := $(shell awk '/define\ $(INPUT_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(INPUT_VER_FILE))
INPUT_SO_VER_MINOR  := $(shell awk '/define\ $(INPUT_VER_PREFIX)_LIB_MINOR/{print $$3}' $(INPUT_VER_FILE))
INPUT_SO_VER_PATCH  := $(shell awk '/define\ $(INPUT_VER_PREFIX)_LIB_PATCH/{print $$3}' $(INPUT_VER_FILE))
INPUT_SO_VER_STRING := $(INPUT_SO_VER_MAJOR).$(INPUT_SO_VER_MINOR).$(INPUT_SO_VER_PATCH)

INPUT_SO_NAME   = $(patsubst %.so,%.so.$(INPUT_SO_VER_MAJOR), $(INPUT_TARGET))
INPUT_SO_TARGET = $(patsubst %.so,%.so.$(INPUT_SO_VER_STRING), $(INPUT_TARGET))

LOCAL_SO_NAME := $(INPUT_SO_NAME)

LOCAL_LDFLAGS = -L$(PREBUILD_3RD_PARTY_DIR)/aac/lib -laacenc -laacdec \
                -L$(PREBUILD_3RD_PARTY_DIR)/libg726/usr/lib -lg726

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)
$(LOCAL_TARGET): prebuild_pulseaudio demuxer-aac.so demuxer-wav.so demuxer-ogg.so
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(INPUT_SO_TARGET)
	@ln -sf $(INPUT_SO_TARGET) $(CAMERA_LIB_DIR)/$(INPUT_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

include $(LOCAL_PATH)/demuxer/codec/make.inc
