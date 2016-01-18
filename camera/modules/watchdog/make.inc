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

ifeq ($(BUILD_AMBARELLA_CAMERA_MODULES_WATCHDOG), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
WATCHDOG_VER_FILE   = $(CAMERA_DIR)/include/watchdog/version.h
WATCHDOG_VER_PREFIX = WATCHDOG

WATCHDOG_TARGET  = libamwatchdog.so
LOCAL_TARGET := $(WATCHDOG_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include/ \
                -I$(CAMERA_DIR)/include -Werror

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

WATCHDOG_SO_VER_MAJOR  := $(shell awk '/define\ $(WATCHDOG_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(WATCHDOG_VER_FILE))
WATCHDOG_SO_VER_MINOR  := $(shell awk '/define\ $(WATCHDOG_VER_PREFIX)_LIB_MINOR/{print $$3}' $(WATCHDOG_VER_FILE))
WATCHDOG_SO_VER_PATCH  := $(shell awk '/define\ $(WATCHDOG_VER_PREFIX)_LIB_PATCH/{print $$3}' $(WATCHDOG_VER_FILE))
WATCHDOG_SO_VER_STRING := $(WATCHDOG_SO_VER_MAJOR).$(WATCHDOG_SO_VER_MINOR).$(WATCHDOG_SO_VER_PATCH)

WATCHDOG_SO_NAME   = $(patsubst %.so,%.so.$(WATCHDOG_SO_VER_MAJOR), $(WATCHDOG_TARGET))
WATCHDOG_SO_TARGET = $(patsubst %.so,%.so.$(WATCHDOG_SO_VER_STRING), $(WATCHDOG_TARGET))

LOCAL_SO_NAME := $(WATCHDOG_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(WATCHDOG_SO_TARGET)
	@ln -sf $(WATCHDOG_SO_TARGET) $(CAMERA_LIB_DIR)/$(WATCHDOG_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif