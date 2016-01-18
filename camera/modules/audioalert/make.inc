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

ifeq ($(BUILD_AMBARELLA_CAMERA_MODULES_AUDIOALERT), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
AUDIOALERT_VER_FILE   = $(CAMERA_DIR)/include/audioalert/version.h
AUDIOALERT_VER_PREFIX = AUDIOALERT

AUDIOALERT_TARGET  = libamaudioalert.so
LOCAL_TARGET := $(AUDIOALERT_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/include \
                -I$(CAMERA_DIR)/include/audioalert \
                -I$(CAMERA_DIR)/include -Werror

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

AUDIOALERT_SO_VER_MAJOR  := $(shell awk '/define\ $(AUDIOALERT_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(AUDIOALERT_VER_FILE))
AUDIOALERT_SO_VER_MINOR  := $(shell awk '/define\ $(AUDIOALERT_VER_PREFIX)_LIB_MINOR/{print $$3}' $(AUDIOALERT_VER_FILE))
AUDIOALERT_SO_VER_PATCH  := $(shell awk '/define\ $(AUDIOALERT_VER_PREFIX)_LIB_PATCH/{print $$3}' $(AUDIOALERT_VER_FILE))
AUDIOALERT_SO_VER_STRING := $(AUDIOALERT_SO_VER_MAJOR).$(AUDIOALERT_SO_VER_MINOR).$(AUDIOALERT_SO_VER_PATCH)

AUDIOALERT_SO_NAME   = $(patsubst %.so,%.so.$(AUDIOALERT_SO_VER_MAJOR), $(AUDIOALERT_TARGET))
AUDIOALERT_SO_TARGET = $(patsubst %.so,%.so.$(AUDIOALERT_SO_VER_STRING), $(AUDIOALERT_TARGET))

LOCAL_SO_NAME := $(AUDIOALERT_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(AUDIOALERT_SO_TARGET)
	@ln -sf $(AUDIOALERT_SO_TARGET) $(CAMERA_LIB_DIR)/$(AUDIOALERT_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif