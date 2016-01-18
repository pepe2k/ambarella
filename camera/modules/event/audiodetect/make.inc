##
## camera/modules/configure/make.inc
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

ifeq ($(BUILD_AMBARELLA_CAMERA_MODULES_EVENT), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

PA_MAJOR = $(shell awk '/define\ PA_MAJOR/{print $$3}' $(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include/pulse/version.h)
PA_MINOR = $(shell awk '/define\ PA_MINOR/{print $$3}' $(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include/pulse/version.h)
PULSE_VERSION = $(PA_MAJOR).$(PA_MINOR)

CAMERA_DIR = $(AMB_TOPDIR)/camera
AUDIO_DETECT_VER_FILE   = $(CAMERA_DIR)/include/event/audiodetect/version.h
AUDIO_DETECT_VER_PREFIX = AUDIO_DETECT

AUDIO_DETECT_TARGET  = audiodetect.so
LOCAL_TARGET := $(AUDIO_DETECT_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/libaudioanalytic/include \
                -I$(CAMERA_DIR)/modules/stream/amf/include \
                -I$(CAMERA_DIR)/modules/stream/include \
                -I$(CAMERA_DIR)/include/event/ \
                -I$(CAMERA_DIR)/include -Werror

LOCAL_LIBS := libamaudioalert.so \
              libamaudioanalysis.so \
              libamstreamamf.so

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/lib/ -lasound \
                 -L$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/usr/lib -lpulsecommon-$(PULSE_VERSION) \
                 -lpulsecore-$(PULSE_VERSION) -lpulse
ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

LOCAL_SO_NAME := $(AUDIO_DETECT_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): prebuild_pulseaudio
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_EVENT_PLUGIN_DIR)/
	@cp -dpRf $< $(CAMERA_EVENT_PLUGIN_DIR)/$(AUDIO_DETECT_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
