##
## camera/modules/stream/engine/make.inc
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
ENGINE_VER_FILE   = $(CAMERA_DIR)/include/stream/version.h
ENGINE_VER_PREFIX = STREAM

ENGINE_TARGET  = libamstreamengine.so
LOCAL_TARGET := $(ENGINE_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/aac/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/include \
                -I$(CAMERA_DIR)/modules/stream/include \
                -I$(CAMERA_DIR)/modules/stream/amf/include \
                -I$(CAMERA_DIR)/modules/stream/core/include \
                -I$(CAMERA_DIR)/modules/stream/input/include \
                -I$(CAMERA_DIR)/modules/stream/output/include \
                -Werror

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

ENGINE_SO_VER_MAJOR  := $(shell awk '/define\ $(ENGINE_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(ENGINE_VER_FILE))
ENGINE_SO_VER_MINOR  := $(shell awk '/define\ $(ENGINE_VER_PREFIX)_LIB_MINOR/{print $$3}' $(ENGINE_VER_FILE))
ENGINE_SO_VER_PATCH  := $(shell awk '/define\ $(ENGINE_VER_PREFIX)_LIB_PATCH/{print $$3}' $(ENGINE_VER_FILE))
ENGINE_SO_VER_STRING := $(ENGINE_SO_VER_MAJOR).$(ENGINE_SO_VER_MINOR).$(ENGINE_SO_VER_PATCH)

ENGINE_SO_NAME   = $(patsubst %.so,%.so.$(ENGINE_SO_VER_MAJOR), $(ENGINE_TARGET))
ENGINE_SO_TARGET = $(patsubst %.so,%.so.$(ENGINE_SO_VER_STRING), $(ENGINE_TARGET))

LOCAL_SO_NAME := $(ENGINE_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(ENGINE_SO_TARGET)
	@ln -sf $(ENGINE_SO_TARGET) $(CAMERA_LIB_DIR)/$(ENGINE_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

