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

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
ADEVICE_VER_FILE   = $(CAMERA_DIR)/include/adevice/version.h
ADEVICE_VER_PREFIX = ADEVICE

ADEVICE_TARGET  = libadevice.so
LOCAL_TARGET := $(ADEVICE_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/include \
                -I$(CAMERA_DIR)/include \
                -I$(CAMERA_DIR)/modules/stream/amf/include \
                -I$(CAMERA_DIR)/include -Werror

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

ADEVICE_SO_VER_MAJOR  := $(shell awk '/define\ $(ADEVICE_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(ADEVICE_VER_FILE))
ADEVICE_SO_VER_MINOR  := $(shell awk '/define\ $(ADEVICE_VER_PREFIX)_LIB_MINOR/{print $$3}' $(ADEVICE_VER_FILE))
ADEVICE_SO_VER_PATCH  := $(shell awk '/define\ $(ADEVICE_VER_PREFIX)_LIB_PATCH/{print $$3}' $(ADEVICE_VER_FILE))
ADEVICE_SO_VER_STRING := $(ADEVICE_SO_VER_MAJOR).$(ADEVICE_SO_VER_MINOR).$(ADEVICE_SO_VER_PATCH)

ADEVICE_SO_NAME   = $(patsubst %.so,%.so.$(ADEVICE_SO_VER_MAJOR), $(ADEVICE_TARGET))
ADEVICE_SO_TARGET = $(patsubst %.so,%.so.$(ADEVICE_SO_VER_STRING), $(ADEVICE_TARGET))

LOCAL_SO_NAME := $(ADEVICE_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)
$(LOCAL_TARGET): prebuild_pulseaudio
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(ADEVICE_SO_TARGET)
	@ln -sf $(ADEVICE_SO_TARGET) $(CAMERA_LIB_DIR)/$(ADEVICE_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

