##
## camera/modules/audioanalysis/make.inc
##
## History:
##    2014/02/13 - [Bin Wang] Created file
##
## Copyright (C) 2014-2018, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_CAMERA_MODULES_AUDIOANALY), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
AUDIOANALY_VER_FILE   = $(CAMERA_DIR)/include/audioanalysis/version.h
AUDIOANALY_VER_PREFIX = AUDIOANALY

AUDIOANALY_TARGET  = libamaudioanalysis.so
LOCAL_TARGET := $(AUDIOANALY_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/libaudioanalytic/include \
                -I$(CAMERA_DIR)/include/audioanalysis \
                -I$(CAMERA_DIR)/modules/configure/include \
                -I$(CAMERA_DIR)/include -Werror

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/libaudioanalytic/usr/lib -laudioanalytic

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

AUDIOANALY_SO_VER_MAJOR  := $(shell awk '/define\ $(AUDIOANALY_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(AUDIOANALY_VER_FILE))
AUDIOANALY_SO_VER_MINOR  := $(shell awk '/define\ $(AUDIOANALY_VER_PREFIX)_LIB_MINOR/{print $$3}' $(AUDIOANALY_VER_FILE))
AUDIOANALY_SO_VER_PATCH  := $(shell awk '/define\ $(AUDIOANALY_VER_PREFIX)_LIB_PATCH/{print $$3}' $(AUDIOANALY_VER_FILE))
AUDIOANALY_SO_VER_STRING := $(AUDIOANALY_SO_VER_MAJOR).$(AUDIOANALY_SO_VER_MINOR).$(AUDIOANALY_SO_VER_PATCH)

AUDIOANALY_SO_NAME   = $(patsubst %.so,%.so.$(AUDIOANALY_SO_VER_MAJOR), $(AUDIOANALY_TARGET))
AUDIOANALY_SO_TARGET = $(patsubst %.so,%.so.$(AUDIOANALY_SO_VER_STRING), $(AUDIOANALY_TARGET))

LOCAL_SO_NAME := $(AUDIOANALY_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(AUDIOANALY_SO_TARGET)
	@ln -sf $(AUDIOANALY_SO_TARGET) $(CAMERA_LIB_DIR)/$(AUDIOANALY_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

