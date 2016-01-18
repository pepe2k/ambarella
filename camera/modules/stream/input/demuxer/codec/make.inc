##
## camera/modules/stream/input/demuxer/codec/make.inc
##
## History:
##    2013/03/13 - [Yupeng Chang] Created file
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

#demuxer-aac
DEMUXER_AAC   = demuxer-aac.so
LOCAL_TARGET := $(DEMUXER_AAC)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/demuxer_aac.cpp \
                           $(LOCAL_PATH)/demuxer.cpp)
LOCAL_CFLAGS := -I$(LOCAL_PATH)         \
                -I$(LOCAL_PATH)/include \
                -I$(CAMERA_DIR)/include \
                -I$(CAMERA_DIR)/modules/stream/include \
                -I$(CAMERA_DIR)/modules/stream/amf/include \
                -I$(CAMERA_DIR)/modules/stream/output/muxer/include \
                -Werror

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

DEMUXER_AAC_SO_NAME   = $(DEMUXER_AAC)
DEMUXER_AAC_SO_TARGET = $(DEMUXER_AAC)

LOCAL_SO_NAME := $(DEMUXER_AAC_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_DEMUXER_DIR)/
	@cp -dpRf $< $(CAMERA_DEMUXER_DIR)/$(DEMUXER_AAC_SO_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

#demuxer-wav
DEMUXER_WAV   = demuxer-wav.so
LOCAL_TARGET := $(DEMUXER_WAV)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/demuxer_wav.cpp \
                           $(LOCAL_PATH)/demuxer.cpp)
LOCAL_CFLAGS := -I$(LOCAL_PATH)         \
                -I$(LOCAL_PATH)/include \
                -I$(CAMERA_DIR)/include \
                -I$(CAMERA_DIR)/modules/stream/include \
                -I$(CAMERA_DIR)/modules/stream/amf/include \
                -I$(CAMERA_DIR)/modules/stream/output/muxer/include \
                -Werror

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

DEMUXER_WAV_SO_NAME   = $(DEMUXER_WAV)
DEMUXER_WAV_SO_TARGET = $(DEMUXER_WAV)

LOCAL_SO_NAME := $(DEMUXER_WAV_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_DEMUXER_DIR)/
	@cp -dpRf $< $(CAMERA_DEMUXER_DIR)/$(DEMUXER_WAV_SO_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

#demuxer-ogg
DEMUXER_OGG   = demuxer-ogg.so
LOCAL_TARGET := $(DEMUXER_OGG)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/demuxer_ogg.cpp \
                           $(LOCAL_PATH)/demuxer.cpp)
LOCAL_CFLAGS := -I$(LOCAL_PATH)         \
                -I$(LOCAL_PATH)/include \
                -I$(CAMERA_DIR)/include \
                -I$(CAMERA_DIR)/modules/stream/include \
                -I$(CAMERA_DIR)/modules/stream/amf/include \
                -I$(CAMERA_DIR)/modules/stream/output/muxer/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/libogg/include \
                -Werror

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/libogg/usr/lib -logg

DEMUXER_OGG_SO_NAME   = $(DEMUXER_OGG)
DEMUXER_OGG_SO_TARGET = $(DEMUXER_OGG)

LOCAL_SO_NAME := $(DEMUXER_OGG_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_DEMUXER_DIR)/
	@cp -dpRf $< $(CAMERA_DEMUXER_DIR)/$(DEMUXER_OGG_SO_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
