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

CAMERA_DIR = $(AMB_TOPDIR)/camera
EVENT_VER_FILE   = $(CAMERA_DIR)/include/event/framework/version.h
EVENT_VER_PREFIX = EVENT

EVENT_TARGET  = libamevent.so
LOCAL_TARGET := $(EVENT_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \
                -I$(CAMERA_DIR)/include/event/framework \
                -I$(CAMERA_DIR)/include -Werror

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

EVENT_SO_VER_MAJOR  := $(shell awk '/define\ $(EVENT_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(EVENT_VER_FILE))
EVENT_SO_VER_MINOR  := $(shell awk '/define\ $(EVENT_VER_PREFIX)_LIB_MINOR/{print $$3}' $(EVENT_VER_FILE))
EVENT_SO_VER_PATCH  := $(shell awk '/define\ $(EVENT_VER_PREFIX)_LIB_PATCH/{print $$3}' $(EVENT_VER_FILE))
EVENT_SO_VER_STRING := $(EVENT_SO_VER_MAJOR).$(EVENT_SO_VER_MINOR).$(EVENT_SO_VER_PATCH)

EVENT_SO_NAME   = $(patsubst %.so,%.so.$(EVENT_SO_VER_MAJOR), $(EVENT_TARGET))
EVENT_SO_TARGET = $(patsubst %.so,%.so.$(EVENT_SO_VER_STRING), $(EVENT_TARGET))

LOCAL_SO_NAME := $(EVENT_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(EVENT_SO_TARGET)
	@ln -sf $(EVENT_SO_TARGET) $(CAMERA_LIB_DIR)/$(EVENT_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif