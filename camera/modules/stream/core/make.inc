##
## camera/modules/stream/core/make.inc
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
CORE_VER_FILE   = $(CAMERA_DIR)/include/stream/version.h
CORE_VER_PREFIX = STREAM

CORE_TARGET  = libamstreamcore.so
LOCAL_TARGET := $(CORE_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/base/*.cpp \
                           $(LOCAL_PATH)/avqueue/*.cpp)
LOCAL_CFLAGS := -I$(LOCAL_PATH)/base \
                -I$(LOCAL_PATH)/avqueue \
                -I$(LOCAL_PATH)/include \
                -I$(CAMERA_DIR)/include \
                -I$(CAMERA_DIR)/modules/stream/amf/include \
                -I$(CAMERA_DIR)/modules/stream/include \
                -Werror

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
  LOCAL_CFLAGS += -O0 -g
endif

CORE_SO_VER_MAJOR  := $(shell awk '/define\ $(CORE_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(CORE_VER_FILE))
CORE_SO_VER_MINOR  := $(shell awk '/define\ $(CORE_VER_PREFIX)_LIB_MINOR/{print $$3}' $(CORE_VER_FILE))
CORE_SO_VER_PATCH  := $(shell awk '/define\ $(CORE_VER_PREFIX)_LIB_PATCH/{print $$3}' $(CORE_VER_FILE))
CORE_SO_VER_STRING := $(CORE_SO_VER_MAJOR).$(CORE_SO_VER_MINOR).$(CORE_SO_VER_PATCH)

CORE_SO_NAME   = $(patsubst %.so,%.so.$(CORE_SO_VER_MAJOR), $(CORE_TARGET))
CORE_SO_TARGET = $(patsubst %.so,%.so.$(CORE_SO_VER_STRING), $(CORE_TARGET))

LOCAL_SO_NAME := $(CORE_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(CORE_SO_TARGET)
	@ln -sf $(CORE_SO_TARGET) $(CAMERA_LIB_DIR)/$(CORE_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

